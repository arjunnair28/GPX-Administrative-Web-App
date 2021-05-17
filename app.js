'use strict'

// C library API
const ffi = require('ffi-napi');

let parserLib = ffi.Library('./libgpxparser', {
  'getGPXDataIfValid': [ 'string', ['string', 'string'] ],
  'getRoutesAndTracksFromFile': ['string', ['string', 'string']],
  'getOtherData': ['string', ['string', 'string', 'int', 'int']],
  'renameRoute': ['int', ['string', 'string', 'int', 'int', 'string']],
  'createEmptyGPX': ['int', ['string', 'string']],
  'addRouteToFile': ['int', ['string', 'string']],
  'addWaypointToRouteInFile': ['int', ['string', 'string']],
  'getRoutesBetweenJSON': ['string', ['string', 'float', 'float', 'float', 'float', 'float']],
  'getTracksBetweenJSON': ['string', ['string', 'float', 'float', 'float', 'float', 'float']],
  'getPathsWithLength': ['string', ['string', 'float']],
  'waypointListToJSON': ['string', ['string', 'int']],
  'lastRouteToJSON': ['string', ['string']]
});

// Express App (Routes)
const express = require("express");
const app     = express();
const path    = require("path");
const fileUpload = require('express-fileupload');
const mysql = require('mysql2/promise');
let connection;

app.use(fileUpload());
app.use(express.static(path.join(__dirname+'/uploads')));

// Minimization
const fs = require('fs');
const JavaScriptObfuscator = require('javascript-obfuscator');

// Important, pass in port as in `npm run dev 1234`, do not change
const portNum = process.argv[2];

// Send HTML at root, do not change
app.get('/',function(req,res){
  res.sendFile(path.join(__dirname+'/public/index.html'));
});

// Send Style, do not change
app.get('/style.css',function(req,res){
  //Feel free to change the contents of style.css to prettify your Web app
  res.sendFile(path.join(__dirname+'/public/style.css'));
});

// Send obfuscated JS, do not change
app.get('/index.js',function(req,res){
  fs.readFile(path.join(__dirname+'/public/index.js'), 'utf8', function(err, contents) {
    const minimizedContents = JavaScriptObfuscator.obfuscate(contents, {compact: true, controlFlowFlattening: true});
    res.contentType('application/javascript');
    res.send(minimizedContents._obfuscatedCode);
  });
});

//Respond to POST requests that upload files to uploads/ directory
app.post('/upload', function(req, res) {
  if(!req.files) {
    return res.status(400).send('No files uploaded');
  }
 
  let uploadFile = req.files.uploadFile;

  // Use the mv() method to place the file somewhere on your server
  uploadFile.mv('uploads/' + uploadFile.name, function(err) {
    if(err) {
      return res.status(500).send(err);
    }

    res.redirect('/');
  });
});

//Respond to GET requests for files in the uploads/ directory
app.get('/uploads/:name', function(req , res){
  fs.stat('uploads/' + req.params.name, function(err, stat) {
    if(err == null) {
      res.sendFile(path.join(__dirname+'/uploads/' + req.params.name));
    } else {
      console.log('Error in file downloading route: '+err);
      res.send('');
    }
  });
});

// Endpoint for getting file information on page load
app.get('/getFiles', function(req , res){
  fs.readdir(__dirname+'/uploads', function(err, files) {
    if (err) {
      console.log(err);
    } else {
      let ret_arr = [];
      // Verify each file with createvalidgpxdoc and validategpxdoc before uploading
      files.forEach(file => {
        if (path.extname(file) == ".gpx") {
          let stringReturned = parserLib.getGPXDataIfValid('uploads/'+file, 'gpx.xsd');
          if (stringReturned == '{}') {
            // If invalid dont upload and just return
            return;
          }
          let gpxInfo = JSON.parse(stringReturned);
          gpxInfo["filename"] = file;
          console.log(gpxInfo);
          console.log(file+' was found');
          ret_arr.push(gpxInfo);
        }
      });
      // Otherwise send information about the uploaded file
      res.send(ret_arr);
    }
  });
});

// Endpoint for getting routes and tracks information from file
app.get('/getFileData', function(req, res) {
  let chosenFile = req.query.filename;
  let fileDataArrayString = parserLib.getRoutesAndTracksFromFile('uploads/'+chosenFile, 'gpx.xsd');
  console.log(fileDataArrayString);
  let fileDataArray = JSON.parse(fileDataArrayString);
  console.log(fileDataArray);
  res.send(fileDataArray);
});

// Endpoint for getting other data for a given route/track
app.get('/getOtherData', function(req, res) {
  let chosenFile = req.query.filename;
  // Type for differentiating routes and tracks
  let type = req.query.type;
  // Index for differentiating between routes and tracks (ordered the same way as the original fle)
  let index = req.query.index;
  let otherDataString = parserLib.getOtherData('uploads/'+chosenFile, 'gpx.xsd', type, index);
  console.log(otherDataString);
  let otherDataArray = JSON.parse(otherDataString);
  res.send(otherDataArray);
});

// Endpoint for renaming a route or track
app.get('/renamePath', function(req, res) {
  let chosenFile = req.query.filename;
  // Type for differentiating routes and tracks
  let type = req.query.type;
  // Index for differentiating between routes and tracks (ordered the same way as the original fle)
  let index = req.query.index;
  let newName = req.query.newName;
  let written = parserLib.renameRoute('uploads/'+chosenFile, 'gpx.xsd', type, index, newName);
  if (written === 0) {
    res.send('Route was not renamed.');
  } else {
    res.send('Route was renamed');
  }
});

// Endpoint for creating a new GPX file
app.get('/createNewGPX', function(req, res) {
  let newFilename = req.query.filename;
  let creator = req.query.creator;
  let created = parserLib.createEmptyGPX('uploads/'+newFilename, creator);
  if (created === 0) {
    res.send('New GPX was not created.');
  } else {
    res.send('New GPX was created.');
  }
});

// Endpoint for adding a new route to a chosen file
app.get('/addRoute', function(req, res) {
  let chosenFile = req.query.filename;
  let routeJSON = req.query.routeJSON;
  let waypointsJSONArray = req.query.waypoints;

  if (parserLib.addRouteToFile('uploads/'+chosenFile, routeJSON) === 0) {
    res.send('New route was not added.');
    console.log('New route was not added');
    return;
  };
  
  if (waypointsJSONArray == undefined || waypointsJSONArray == null) {
    waypointsJSONArray = [];
  }

  if (waypointsJSONArray.length > 0) {
    waypointsJSONArray.forEach(waypoint => {
      console.log(waypoint);
      if (parserLib.addWaypointToRouteInFile('uploads/'+chosenFile, waypoint) === 0) {
        res.send('One or more waypoints could not be added.');
        return;
      };
    });
  }

  res.send('Route added successfully.');

});

// Endpoint for finding all paths between two points
app.get('/findPaths', function(req, res) {
  let filenames = req.query.filenames;
  let lat1 = req.query.lat1;
  let lon1 = req.query.lon1;
  let lat2 = req.query.lat2;
  let lon2 = req.query.lon2;
  let delta = req.query.delta;

  let retObject = {};
  let routesArray = [];
  let tracksArray = [];

  // For each file get the matching routes and matching tracks
  filenames.forEach(filename => {
    let returnedJSONRoutes = parserLib.getRoutesBetweenJSON('uploads/'+filename, lat1, lon1, lat2, lon2, delta);
    routesArray = routesArray.concat(JSON.parse(returnedJSONRoutes));
    console.log(routesArray);
    let returnedJSONTracks = parserLib.getTracksBetweenJSON('uploads/'+filename, lat1, lon1, lat2, lon2, delta);
    tracksArray = tracksArray.concat(JSON.parse(returnedJSONTracks));
    console.log(tracksArray);
  });

  // Add them to the return object
  retObject["routes"] = routesArray;
  retObject["tracks"] = tracksArray;
  res.send(retObject);

});

// Endpoint for finding all paths with a specific length
app.get('/findPathsWithLength', function(req, res) {
  let filenames = req.query.filenames;
  let length = req.query.length;

  let returnNums = {};
  returnNums["totalForRoutes"] = 0;
  returnNums["totalForTracks"] = 0;

  // For each file count the number of matched paths and add it to the total
  filenames.forEach(filename => {
    let returnedJSON = parserLib.getPathsWithLength('uploads/'+filename, length);
    let tmpObject = JSON.parse(returnedJSON);
    returnNums["totalForRoutes"] += tmpObject["rt"];
    returnNums["totalForTracks"] += tmpObject["tr"];
  });

  returnNums["total"] = returnNums["totalForRoutes"] + returnNums["totalForTracks"];
  res.send(returnNums);

});

// Endpoint for logging in to database and creating tables if they do not exist
app.get('/loginToDatabase', async function(req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  console.log(dbUsername, dbPassword, dbName);
  // Starting connection, with error checking
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    // Try and create all the tables with the provided schema
    try {
      await connection.execute(`
      CREATE TABLE IF NOT EXISTS FILE 
      (
        gpx_id INT AUTO_INCREMENT,
        file_name VARCHAR(60) NOT NULL,
        ver DECIMAL(2,1) NOT NULL,
        creator VARCHAR(256) NOT NULL,
        PRIMARY KEY(gpx_id)
      )
      `);
    } catch (e2) {
      console.log('Query error: ' + e2);
      res.send('Failed to make FILE table');
    }
    try {
      await connection.execute(`
      CREATE TABLE IF NOT EXISTS ROUTE 
      (
        route_id INT AUTO_INCREMENT,
        route_name VARCHAR(256),
        route_len FLOAT(15,7) NOT NULL,
        gpx_id INT NOT NULL,
        PRIMARY KEY(route_id),
        FOREIGN KEY(gpx_id) REFERENCES FILE(gpx_id) ON DELETE CASCADE
      )
      `);
    } catch (e2) {
      console.log('Query error: ' + e2);
      res.send('Failed to make ROUTE table');
    }
    try {
      await connection.execute(`
      CREATE TABLE IF NOT EXISTS POINT 
      (
        point_id INT AUTO_INCREMENT,
        point_index INT NOT NULL,
        latitude DECIMAL(11,7) NOT NULL,
        longitude DECIMAL(11,7) NOT NULL,
        point_name VARCHAR(256),
        route_id INT NOT NULL,
        PRIMARY KEY(point_id),
        FOREIGN KEY(route_id) REFERENCES ROUTE(route_id) ON DELETE CASCADE
      )
      `);
    } catch (e2) {
      console.log('Query error: ' + e2);
      res.send('Failed to make POINT table');
    }
    res.send('Connection successful!');
  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    // End the connection
    if (connection && connection.end) connection.end();
  }
});

// Endpoint for storing all files on the server into the database
app.get('/storeFiles', async function(req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let filenames = req.query.fileList;
  // Try and connect again with same credentials
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });

    for (let file of filenames) {

      try {
        let [fileRows, ] = await connection.execute(`
        SELECT COUNT(FILE.gpx_id) AS FILE_COUNT FROM FILE WHERE FILE.file_name = "`+file+`"`
        );
        // If the file already is in the database, skip to next iteration
        if (fileRows[0].FILE_COUNT > 0) {
          continue;
        }
      } catch (e1) {
        console.log('Query error: ' + e1);
        res.send('Failed to get count from FILE table');
      }
      
      // Get the file data to place in the database fields
      let stringReturned = parserLib.getGPXDataIfValid('uploads/'+file, 'gpx.xsd');
      if (stringReturned == '{}') {
        return;
      }
      let gpxInfo = JSON.parse(stringReturned);
      gpxInfo["filename"] = file;

      try {
        // Insert the new record into the file database
        await connection.execute(`
        INSERT INTO FILE (file_name, ver, creator)
        VALUES("`+file+`", `+gpxInfo["version"]+`, "`+gpxInfo["creator"]+`")
        `);
        let gpxID;

        try {
          // Get the gpx ID of the newly inserted file (just in case the auto increment is not reset)
          let [gpxIDRows, ] = await connection.execute(`
          SELECT (FILE.gpx_id) FROM FILE
          WHERE FILE.file_name = "`+file+`"
          `);
          gpxID = gpxIDRows[0].gpx_id;
        } catch (e1) {
          console.log('Query error: ' + e1);
          res.send('Failed to check if file exists in FILE table');
        }
        
        // Get the route information from the file
        let fileDataArrayString = parserLib.getRoutesAndTracksFromFile('uploads/'+file, 'gpx.xsd');
        let fileDataArray = JSON.parse(fileDataArrayString);
        let routesArray = fileDataArray["routes"];

        let i = 0;
        for (let route of routesArray) {
          try {
            // If the route has no name, default value is NULL
            if (route["name"] == "") {
              await connection.execute(`
              INSERT INTO ROUTE (route_name, route_len, gpx_id)
              VALUES(NULL, `+route["len"]+`, `+gpxID+`)
              `);
            } else {
              await connection.execute(`
              INSERT INTO ROUTE (route_name, route_len, gpx_id)
              VALUES("`+route["name"]+`", `+route["len"]+`, `+gpxID+`)
              `);
            }
            
            try {

              let routeID;

              // Get route ID of newly added route
              try {
                let [routeIDrows, ] = await connection.execute(`
                SELECT (ROUTE.route_id) FROM ROUTE
                WHERE ROUTE.route_name = "`+route["name"]+`" AND ROUTE.route_len = `+route["len"]
                );
                routeID = routeIDrows[0].route_id;
              } catch (e3) {
                console.log('Query error: ' + e3);
                res.send('Failed to find route from ROUTE table');
              }

              // Get all the route points from the route in the file, with the given index
              let waypointArrayString = parserLib.waypointListToJSON('uploads/'+file, i);
              let waypointArray = JSON.parse(waypointArrayString);

              let j = 0;
              for (let point of waypointArray) {

                try {
                  // If point has no name, default value is NULL
                  if (point["name"] == "") {
                    await connection.execute(`
                    INSERT INTO POINT (point_index, point_name, latitude, longitude, route_id)
                    VALUES(`+j+`, NULL, `+point["latitude"]+`, `+point["longitude"]+`, `+routeID+`)
                    `);
                  } else {
                    await connection.execute(`
                    INSERT INTO POINT (point_index, point_name, latitude, longitude, route_id)
                    VALUES(`+j+`, "`+point["name"]+`", `+point["latitude"]+`, `+point["longitude"]+`, `+routeID+`)
                    `);
                  }

                } catch (e3) {
                  console.log('Query error: ' + e3);
                  res.send('Failed to insert point into POINT table');
                }

                j++;
              }

            } catch (e2) {
              console.log('Query error: ' + e2);
              res.send('Failed to insert point into POINT table');
            }

          } catch (e1) {
            console.log('Query error: ' + e1);
            res.send('Failed to insert route into ROUTE table');
          }

          i++;

        }

      } catch (e) {
        console.log('Query error: ' + e);
        res.send('Failed to insert file into FILE table');
      }

    };

    res.send('Added successfully!');

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to clear every record from the database
app.get('/clearAllRows', async function(req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });

    // Since we have 'ON DELETE CASCADE', we just have to delete from the FILE table, which in turn deletes from the ROUTE table, which in turn deletes from the POINT table
    try {
      await connection.execute(`
      DELETE FROM FILE`
      );
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Clearing failed!');
    }

    res.send('Cleared successfully!');

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
  
});

// Endpoint to display current number of records in each table of the database (FILE, ROUTE, POINT)
app.get('/displayStatus', async function(req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    
    let retString = 'Database has';

    try {
      let [fileRows, fileField] = await connection.execute(`
      SELECT COUNT(FILE.gpx_id) AS FILE_COUNT FROM FILE
      `);
      retString += ' '+fileRows[0].FILE_COUNT+' files,';
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to get count from FILE table');
    }

    try {
      let [routeRows, routeField] = await connection.execute(`
      SELECT COUNT(ROUTE.route_id) AS ROUTE_COUNT FROM ROUTE
      `);
      retString += ' '+routeRows[0].ROUTE_COUNT+' routes,';
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to get count from ROUTE table');
    }

    try {
      let [pointRows, pointField] = await connection.execute(`
      SELECT COUNT(POINT.point_id) AS ROUTE_COUNT FROM POINT
      `);
      retString += ' '+pointRows[0].ROUTE_COUNT+' points';
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to get count from ROUTE table');
    }

    res.send(retString);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to rename a specific route from a specific file (this time in the database)
app.get('/renameRoute', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let file = req.query.filename;
  let index = req.query.index;
  let newName = req.query.newName;

  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });

    try {
      // Find the route by ordering by route_id (since it is not guranteed to get routes back in order), and offset by the index, because of auto increment
      let [routeRows, ] = await connection.execute(`
      SELECT route_id FROM ROUTE
      WHERE gpx_id IN
      (
        SELECT gpx_id FROM FILE
        WHERE file_name = "`+file+`"
      )
      ORDER BY route_id
      LIMIT 1 OFFSET `+(index-1)
      );
      try {
        await connection.execute(`
        UPDATE ROUTE
        SET route_name = "`+newName+`"
        WHERE route_id = `+routeRows[0].route_id
        );
      } catch (e2) {
        console.log('Query error: ' + e2);
        res.send('Failed to rename route');
      }
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find route to rename');
    }

    res.send('Renamed successfully!');

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to add a route to the database
app.get('/addRouteDatabase', async function(req, res) {

  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;

  let chosenFile = req.query.filename;
  let route = JSON.parse(parserLib.lastRouteToJSON('uploads/'+chosenFile));
  let waypointArray = req.query.waypoints;
  if (waypointArray == undefined || waypointArray == null) {
    waypointArray = [];
  }
  let newArray = [];
  if (waypointArray.length > 0) {
    waypointArray.forEach(waypoint => {
      console.log(waypoint);
      newArray.push(JSON.parse(waypoint));
    });
  }

  waypointArray = newArray;

  console.log(waypointArray);

  let gpxID;

  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });

    // Same process as before
    try {
      let [gpxIDRows, ] = await connection.execute(`
      SELECT (FILE.gpx_id) FROM FILE
      WHERE FILE.file_name = "`+chosenFile+`"
      `);
      gpxID = gpxIDRows[0].gpx_id;
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to check if file exists in FILE table');
    }
  
    try {
      if (route["name"] == "") {
        await connection.execute(`
        INSERT INTO ROUTE (route_name, route_len, gpx_id)
        VALUES(NULL, `+route["len"]+`, `+gpxID+`)
        `);
      } else {
        await connection.execute(`
        INSERT INTO ROUTE (route_name, route_len, gpx_id)
        VALUES("`+route["name"]+`", `+route["len"]+`, `+gpxID+`)
        `);
      }
      try {
  
        let routeID;
  
        try {
          let [routeIDrows, ] = await connection.execute(`
          SELECT (ROUTE.route_id) FROM ROUTE
          WHERE ROUTE.route_name = "`+route["name"]+`" AND ROUTE.route_len = `+route["len"]
          );
          routeID = routeIDrows[0].route_id;
        } catch (e3) {
          console.log('Query error: ' + e3);
          res.send('Failed to find route from ROUTE table');
        }
  
        let j = 0;
        for (let point of waypointArray) {
          try {
            await connection.execute(`
            INSERT INTO POINT (point_index, point_name, latitude, longitude, route_id)
            VALUES(`+j+`, NULL, `+point["lat"]+`, `+point["lon"]+`, `+routeID+`)
            `);
          } catch (e3) {
            console.log('Query error: ' + e3);
            res.send('Failed to insert point into POINT table');
          }
          j++;
        }
  
      } catch (e2) {
        console.log('Query error: ' + e2);
        res.send('Failed to insert point into POINT table');
      }
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to insert route into ROUTE table');
    }

    res.send('Success!');

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }

});

// Endpoint to display all the routes in the ROUTE table
app.get('/displayAllRoutes', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let order = req.query.order;
  let sortType = req.query.sortType;
  let isSorted = req.query.isSorted;
  console.log(isSorted);
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let routeRows;
    try {
      if (isSorted == 'true') {
        [routeRows, ] = await connection.execute(`
        SELECT ROUTE.route_id, ROUTE.route_name, ROUTE.route_len, FILE.file_name
        FROM ROUTE INNER JOIN FILE
        ON ROUTE.gpx_id = FILE.gpx_id
        ORDER BY ROUTE.`+sortType+` `+order
        );
      } else {
        [routeRows, ] = await connection.execute(`
        SELECT ROUTE.route_id, ROUTE.route_name, ROUTE.route_len, FILE.file_name
        FROM ROUTE INNER JOIN FILE
        ON ROUTE.gpx_id = FILE.gpx_id
        `);
      }

    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find route to rename');
    }
    console.log(routeRows);
    res.send(routeRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to display specific routes from a specific file
app.get('/displaySpecificRoutes', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let order = req.query.order;
  let sortType = req.query.sortType;
  let isSorted = req.query.isSorted;
  let filename = req.query.filename;
  console.log(isSorted);
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let routeRows;
    try {
      if (isSorted == 'true') {
        [routeRows, ] = await connection.execute(`
        SELECT ROUTE.route_id, ROUTE.route_name, ROUTE.route_len, FILE.file_name
        FROM ROUTE INNER JOIN FILE
        ON ROUTE.gpx_id = FILE.gpx_id
        WHERE FILE.file_name = "`+filename+`" 
        ORDER BY ROUTE.`+sortType+` `+order
        );
      } else {
        [routeRows, ] = await connection.execute(`
        SELECT ROUTE.route_id, ROUTE.route_name, ROUTE.route_len, FILE.file_name
        FROM ROUTE INNER JOIN FILE
        ON ROUTE.gpx_id = FILE.gpx_id
        WHERE FILE.file_name = "`+filename+`" 
        `);
      }

    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find route to rename');
    }
    // console.log(routeRows);
    res.send(routeRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to display all the points in the POINT table
app.get('/displayAllPoints', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let routeID = req.query.routeID;
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let pointRows;
    try {
      [pointRows, ] = await connection.execute(`
      SELECT point_id, point_name, latitude, longitude, point_index, route_id
      FROM POINT
      WHERE POINT.route_id = `+routeID+` 
      ORDER BY POINT.point_index
      `);
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find route to rename');
    }
    // console.log(routeRows);
    res.send(pointRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to display all points from a specific file
app.get('/displaySpecificPoints', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let order = req.query.order;
  let sortType = req.query.sortType;
  let isSorted = req.query.isSorted;
  let filename = req.query.filename;
  console.log(isSorted);
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let pointRows;
    try {
      if (isSorted == 'true') {
        [pointRows, ] = await connection.execute(`
        SELECT POINT.point_id, POINT.point_name, POINT.latitude, POINT.longitude, POINT.point_index, POINT.route_id
        FROM POINT INNER JOIN ROUTE
        ON POINT.route_id = ROUTE.route_id
        WHERE ROUTE.route_id IN
        (
          SELECT ROUTE.route_id
          FROM ROUTE INNER JOIN FILE
          ON ROUTE.gpx_id = FILE.gpx_id
          WHERE FILE.file_name = "`+filename+`"
        )
        ORDER BY ROUTE.`+sortType+` `+order+`, POINT.point_index
        `);
      } else {
        [pointRows, ] = await connection.execute(`
        SELECT POINT.point_id, POINT.point_name, POINT.latitude, POINT.longitude, POINT.point_index, POINT.route_id
        FROM POINT INNER JOIN ROUTE
        ON POINT.route_id = ROUTE.route_id
        WHERE ROUTE.route_id IN
        (
          SELECT ROUTE.route_id
          FROM ROUTE INNER JOIN FILE
          ON ROUTE.gpx_id = FILE.gpx_id
          WHERE FILE.file_name = "`+filename+`"
        )
        ORDER BY POINT.route_id, POINT.point_index
        `);
      }

    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find points');
    }
    // console.log(routeRows);
    res.send(pointRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to get the longest/shortest N routes (ordered by length) from a specific file
app.get('/getNRoutesFromFile', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  let order = req.query.order;
  let sortType = req.query.sortType;
  let isSorted = req.query.isSorted;
  let filename = req.query.filename;
  let numRecords = req.query.numRecords;
  let type = req.query.type;
  console.log(isSorted);
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let routeRows;
    try {
      if (isSorted == 'true') {
        [routeRows, ] = await connection.execute(`
          SELECT ROUTE.route_id, ROUTE.route_name, ROUTE.route_len, FILE.file_name
          FROM ROUTE INNER JOIN FILE
          ON ROUTE.gpx_id = FILE.gpx_id
          WHERE FILE.file_name = "`+filename+`"
          ORDER BY ROUTE.route_len `+type+`
          LIMIT `+numRecords
        );
        if (order == 'ASC') {
          routeRows.sort((a, b) => (a[sortType] > b[sortType]) ? 1 : -1);
        } else {
          routeRows.sort((a, b) => (a[sortType] < b[sortType]) ? 1 : -1);
        }
      } else {
        [routeRows, ] = await connection.execute(`
          SELECT ROUTE.route_id, ROUTE.route_name, ROUTE.route_len, FILE.file_name
          FROM ROUTE INNER JOIN FILE
          ON ROUTE.gpx_id = FILE.gpx_id
          WHERE FILE.file_name = "`+filename+`"
          ORDER BY ROUTE.route_len `+type+`
          LIMIT `+numRecords
        );
      }

    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find route to rename');
    }
    // console.log(routeRows);
    res.send(routeRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to list all the files in the FILE table
app.get('/listFilesInDatabase', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let fileRows;
    try {
      [fileRows, ] = await connection.execute(`
      SELECT file_name
      FROM FILE
      `);
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find files');
    }
    console.log(fileRows);
    res.send(fileRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

// Endpoint to list all the routes from the ROUTE table
app.get('/listRteIDs', async function (req, res) {
  let dbUsername = req.query.username;
  let dbPassword = req.query.password;
  let dbName = req.query.name;
  try {
    connection = await mysql.createConnection({
      host: 'dursley.socs.uoguelph.ca',
      user: dbUsername,
      password: dbPassword,
      database: dbName
    });
    let routeRows;
    try {
      [routeRows, ] = await connection.execute(`
      SELECT route_id
      FROM ROUTE
      `);
    } catch (e1) {
      console.log('Query error: ' + e1);
      res.send('Failed to find routes');
    }
    console.log(routeRows);
    res.send(routeRows);

  } catch (e) {
    console.log('Query error: ' + e);
    res.send('Connection failed!');
  } finally {
    if (connection && connection.end) connection.end();
  }
});

app.listen(portNum);
console.log('Running app at localhost: ' + portNum);
