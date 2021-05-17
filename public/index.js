// Function to update the GPX file view, since it can be done multiple times
function updateFileTable () {
    // Ajax call to C function wrapper to get file information from files in /uploads
    $.ajax({
        type: 'get',
        dataType: 'json',
        url: '/getFiles',
        success: function (data) {
            // If no files
            if (data.length == 0) {
                return;
            }
            // Table header
            let table = `
            <thead class="thead-light">
            <tr>
                <th class="align-middle">File name (click to download)</th>
                <th class="align-middle">Version</th>
                <th class="align-middle">Creator</th>
                <th class="align-middle">Number of waypoints</th>
                <th class="align-middle">Number of routes</th>
                <th class="align-middle">Number of tracks</th>
            </tr>
            </thead>`;
            // For each file add a record to the table as well as the filename to the dropdowns if it is not already present
            let i = 1;
            data.forEach(file => {
                if (JSON.stringify(file) === '{}') {
                    return;
                }
                table += `
                <tr>
                    <td class="align-middle fileName"><a href="`+file["filename"]+`" download>`+file["filename"]+`</a></td>
                    <td class="align-middle">`+file["version"]+`</td>
                    <td class="align-middle">`+file["creator"]+`</td>
                    <td class="align-middle">`+file["numWaypoints"]+`</td>
                    <td id="FileNo`+i+`" class="align-middle">`+file["numRoutes"]+`</td>
                    <td class="align-middle">`+file["numTracks"]+`</td>
                </tr>`
                let exists = false;
                $('#fileChosen option').each(function(){
                    if (this.value == file["filename"]) {
                        exists = true;
                        return false;
                    }
                });
                if (exists == false) {
                    $('#fileChosen').append('<option value="'+file["filename"]+'">'+file["filename"]+'</option>');
                }
                exists = false;
                $('#fileChosen2 option').each(function(){
                    if (this.value == file["filename"]) {
                        exists = true;
                        return false;
                    }
                });
                if (exists == false) {
                    $('#fileChosen2').append('<option value="'+file["filename"]+'">'+file["filename"]+'</option>');
                }
            });
            // Add the upload button to the bottom of the table
            table += `
            <tr>
                <td colspan="6" id="uploadCell">
                    <form ref='uploadFileForm'
                        id='uploadFileForm'
                        action='/upload'
                        method='post'
                        encType="multipart/form-data">
                        <div class="form-group">
                            <input type="file" id="fileName" name="uploadFile" class="btn btn-outline-dark">
                            <input type="submit" value="Upload file" class="btn btn-outline-dark">
                        </div>
                    </form>
                </td>
            </tr>`;
            $('#fileTable').html(table);
            console.log('Initial file load was successful'); 

        },
        fail: function(error) {
            error.preventDefault();
            alert(error);
        }
    });
}

// Global variables to hold the username and password of the database

let loggedIn = 0;

let username;
let password;
let name;

// On page load
$(document).ready(function() {

    // Update file table
    updateFileTable();

    // When a file is chosen from the dropdown menu
    $('#fileChosen').change(function(e) {
        e.preventDefault();
        let chosenFile = this.value;
        // Ajax call to get the file's data
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/getFileData',
            data: {
                filename: chosenFile
            },
            success: function(data) {
                let routesArray = data["routes"];
                let tracksArray = data["tracks"];
                // If there are no routes or tracks, display a message instead of the table headers with no rows
                if (routesArray.length === 0 && tracksArray.length === 0) {
                    $('#pathDataFromFile').html(`<tr>
                                                    <td><i>The selected file has no routes or tracks</i></td>
                                                </tr>`);
                    return;
                }
                // Table headers
                let table = `
                    <thead class="thead-light">
                        <tr>
                            <th class="align-middle">Component</th>
                            <th class="align-middle">Name</th>
                            <th class="align-middle">Number of points</th>
                            <th class="align-middle">Length</th>
                            <th class="align-middle">Loop</th>
                        </tr>
                    </thead>`;
                // Route information first
                let i = 1;
                routesArray.forEach(route => {
                    table += '<tr>';
                    table += '<td class="align-middle">Route ' + i + '</td>';
                    let name = '';
                    for (key in route) {
                        if (key == 'loop') {
                            if (route[key] == false) {
                                table += '<td class="align-middle" id="Route'+ key + i +'">FALSE</td>';
                            } else {
                                table += '<td class="align-middle" id="Route'+ key + i +'">TRUE</td>';
                            }
                        } else if (key == 'len') {
                            table += '<td class="align-middle" id="Route'+ key + i +'">' + route[key] + 'm</td>';
                        } else {
                            if (key == 'name') {
                                name = route[key];
                            }
                            table += '<td class="align-middle" id="Route'+ key + i +'">' + route[key] + '</td>';
                        }
                    };
                    table += `
                        <td class="align-middle">
                            <button data-filename="`+chosenFile+`" data-routename="`+name+`" class="btn btn-sm btn-outline-secondary routeOther" id="Route`+i+`Other">Show Other Data</button>
                        </td>
                        <td class="align-middle">
                            <button data-filename="`+chosenFile+`" data-routename="`+name+`" class="btn btn-sm btn-outline-secondary routeRename" id="Route`+i+`Rename">Rename</button>
                        </td>`;
                    table += '</tr>';
                    i += 1;
                });
                // Reset index, then loop for track information
                i = 1;
                tracksArray.forEach(track => {
                    table += '<tr>';
                    table += '<td class="align-middle">Track ' + i + '</td>';
                    let name = '';
                    for (key in track) {
                        if (key == 'loop') {
                            if (track[key] == false) {
                                table += '<td class="align-middle" id="Track'+ key + i +'">FALSE</td>';
                            } else {
                                table += '<td class="align-middle" id="Track'+ key + i +'">TRUE</td>';
                            }
                        } else if (key == 'len') {
                            table += '<td class="align-middle" id="Track'+ key + i +'">' + track[key] + 'm</td>';
                        } else {
                            if (key == 'name') {
                                name = track[key];
                            }
                            table += '<td class="align-middle" id="Track'+ key + i +'">' + track[key] + '</td>';
                        }
                    };
                    table += `
                        <td class="align-middle">
                            <button data-filename="`+chosenFile+`" data-trackname="`+name+`" class="btn btn-sm btn-outline-secondary trackOther" id="Track`+i+`Other">Show Other Data</button>
                        </td>
                        <td class="align-middle">
                            <button data-filename="`+chosenFile+`" data-trackname="`+name+`" class="btn btn-sm btn-outline-secondary trackRename" id="Track`+i+`Rename">Rename</button>
                        </td>`;
                    table += '</tr>';
                    i += 1;
                });
                $('#pathDataFromFile').html(table);
            },
            fail: function(error) {
                error.preventDefault();
                alert(error);
            }
        });
    });

    // Get the other data for an individual route
    $('#pathDataFromFile').on('click', '.routeOther', function(e){
        e.preventDefault();
        // Index is always formatted the same 'Route' + the index number, so remove first 5 characters
        let routeIndex = this.id.slice(5, -5);
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/getOtherData',
            data: {
                filename: this.dataset.filename,
                type: 1,
                index: routeIndex
            },
            success: function(data) {
                // If the route has no other data, display an appropriate message, otherwise display otherdata, separated by newlines
                if (JSON.stringify(data) === '[]') {
                    alert('No other data for this route!');
                } else {
                    let otherDataString = 'Other data for this route:\n';
                    data.forEach(otherData => {
                        otherDataString += '\n' + otherData["name"] + ': ' + otherData["value"] + '\n';
                    });
                    // Show the other data in an alert
                    alert(otherDataString);
                }
            },
            fail: function(error) {
                error.preventDefault();
                alert('Could not retrieve other data.');
            }
        });
    });

    // Rename a route in the table
    $('#pathDataFromFile').on('click', '.routeRename', function(e){
        e.preventDefault();
        let routeIndex = this.id.slice(5, -6);
        // Prompt user for a new name
        let nameEntered = prompt('What do you want to rename this route to?');
        let filename = this.dataset.filename;
        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/renamePath',
            data: {
                filename: filename,
                type: 1,
                index: routeIndex,
                newName: nameEntered
            },
            success: function(data) {
                if (data == 'Route was not renamed.') {
                    alert(data);
                } else {
                    // If the route was renamed in the file (by C function), rename it in the database too
                    $('#Routename'+routeIndex).text(nameEntered);
                    if (loggedIn === 1) {
                        $.ajax({
                            type: 'get',
                            dataType: 'text',
                            url: '/renameRoute',
                            data: {
                                filename: filename,
                                username: username,
                                password: password,
                                name: name,
                                index: routeIndex,
                                newName: nameEntered
                            },
                            success: function(data1) {
                                if (data1 == 'Renamed successfully!') {
                                } else {
                                    alert(data1 + '\nPlease try again.');
                                }
                            },
                            fail: function(error1) {
                                error1.preventDefault();
                                alert('Route was not renamed in database.');
                            }
                        });
                    }
                }
            },
            fail: function(error) {
                error.preventDefault();
                alert('Route was not renamed.');
            }
        });
    });

    // Get the other data for track
    $('#pathDataFromFile').on('click', '.trackOther', function(e){
        e.preventDefault();
        let trackIndex = this.id.slice(5, -5);
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/getOtherData',
            data: {
                filename: this.dataset.filename,
                type: 2,
                index: trackIndex
            },
            success: function(data) {
                if (JSON.stringify(data) === '[]') {
                    alert('No other data for this track!');
                } else {
                    let otherDataString = 'Other data for this track:\n';
                    data.forEach(otherData => {
                        otherDataString += '\n' + otherData["name"] + ': ' + otherData["value"] + '\n';
                    });
                    alert(otherDataString);
                }
            },
            fail: function(error) {
                error.preventDefault();
                alert('Could not retrieve other data.');
            }
        });
    });

    // Rename a track in the table
    $('#pathDataFromFile').on('click', '.trackRename', function(e){
        e.preventDefault();
        let trackIndex = this.id.slice(5, -6);
        let nameEntered = prompt('What do you want to rename this track to?');
        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/renamePath',
            data: {
                filename: this.dataset.filename,
                type: 2,
                index: trackIndex,
                newName: nameEntered
            },
            success: function(data) {
                if (data == 'Route was not renamed.') {
                    alert('Track was not renamed.');
                } else {
                    $('#Trackname'+trackIndex).text(nameEntered);
                }
            },
            fail: function(error) {
                error.preventDefault();
                alert('Track was not renamed.');
            }
        });
    });

    // Create a new GPX file
    $('#createForm').submit(function(e){
        e.preventDefault();
        let gpxDoc = {};
        let fileName = this.elements["filename"].value;
        // Remove all present error messages
        $('.error').remove();
        // Check file extension
        if (fileName.slice(-4) != ".gpx") {
            $('#newFileName').after('<small class="error">Must have a .gpx extension</small>');
            return;
        }
        // Name has to be longer than '.gpx'
        if (fileName.length <= 4) {
            $('#newFileName').after('<small class="error">Please enter a valid file name</small>');
            return;
        }
        // Check if creator is empty
        let creator = this.elements["creator"].value;
        if (gpxDoc["creator"] === "") {
            $('#creator').after('<small class="error">Cannot be empty</small>');
            return;
        }

        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/createNewGPX',
            data: {
                filename: fileName,
                creator: creator
            },
            success: function (data) {
                if (data == "New GPX was not created.") {
                    alert(data);
                } else {
                    // If file was created successfully, update the file table
                    updateFileTable();
                }
            },
            fail: function(error) {
                error.preventDefault();
                alert(error);
            }
        });
     
    });

    // When the + button for adding more waypoints in the 'Add Route' section is changed
    $('#newRouteNumPoints').change(function(e) {
        let numPoints = this.value;
        let form = $("#addForm");
        let startIndex = $('.extraPoint').length;
        // If the current value is greater than the number of input fields, then more needs to be added
        if (numPoints > startIndex) {
            // Temporarily remove the add route button, so that the extra rows aren't added after
            $("#addRouteButton").remove();
            // Add as many rows as needed
            for (i = startIndex + 1; i <= numPoints; i++) {
                form.append(`
                <div class="form-row align-items-center extraPoint">
                    <div class="col-auto">
                        <input type="text" readonly class="form-control-plaintext" value="Route point `+i+`">
                    </div>
                    <div class="col">
                        <input id="Latitude`+i+`" name="Latitude`+i+`"type="text" class="form-control" placeholder="Latitude" required>
                    </div>
                    <div class="col">
                        <input id="Longitude`+i+`" name="Longitude`+i+`"type="text" class="form-control" placeholder="Longitude" required>
                    </div>
                </div>`);
            }
            form.append(`<div>
                            <input id="addRouteButton" type="submit" class="btn btn-outline-dark form-control extraButton" value="Add route">
                        </div>`);
        } else if (numPoints < startIndex) {
            // Otherwise remove the amount of extra rows to a minimum of 0
            let numRowsToRemove = startIndex - numPoints;
            $(".extraPoint:last").slice(-numRowsToRemove).remove();
        }
    });

    // Adding a route to an existing GPX file
    $('#addForm').submit(function(e){
        e.preventDefault();
        let chosenFile = $("#fileChosen2").find(":selected").text();
        let routeName = this.elements["newRouteName"].value;
        let route = {}
        route["name"] = routeName;
        if (routeName == null || routeName == "") {
            route["name"] = " ";
        }
        let routeJSON = JSON.stringify(route);
        let numPoints = this.elements["newRouteNumPoints"].value;
        let waypointsArr = [];
        // Remove all error messages
        $('.error').remove();
        let errorCount = 0;
        // Check validity of all the points
        for (i = 1; i <= numPoints; i++) {
            let tmpWaypoint = {};
            tmpWaypoint["lat"] = this.elements["Latitude"+i].value;
            tmpWaypoint["lon"] = this.elements["Longitude"+i].value;

            // Latitude must be between -90 and 90
            if (tmpWaypoint["lat"] < -90 || tmpWaypoint["lat"] > 90 || !$.isNumeric(tmpWaypoint["lat"])) {
                $('#Latitude'+i).after('<small class="error">Must be a number between -90 and 90</small>');
                errorCount++;
            }

            // Longitude must be between -180 and 180
            if (tmpWaypoint["lon"] < -180 || tmpWaypoint["lon"] > 180 || !$.isNumeric(tmpWaypoint["lon"])) {
                $('#Longitude'+i).after('<small class="error">Must be a number between -180 and 180</small>');
                errorCount++;
            }

            // Parse as float, for the C function to be able to read it properly
            tmpWaypoint["lat"] = parseFloat(tmpWaypoint["lat"]);
            tmpWaypoint["lon"] = parseFloat(tmpWaypoint["lon"]);

            let waypointStr = JSON.stringify(tmpWaypoint);
            waypointsArr.push(waypointStr);
        }

        // If there are any errors, don't proceed
        if (errorCount > 0) {
            return;
        }

        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/addRoute',
            data: {
                filename: chosenFile,
                routeJSON: routeJSON,
                waypoints: waypointsArr
            },
            success: function(data) {
                if (data == 'New route was not added.' || data == 'One or more waypoints could not be added.') {
                    alert(data);
                } else {
                    // If the route was added to the file successfully, add it to the database too
                    if (loggedIn === 1) {
                        $.ajax({
                            type: 'get',
                            dataType: 'text',
                            url: '/addRouteDatabase',
                            data: {
                                username: username,
                                password: password,
                                name: name,
                                filename: chosenFile,
                                routeJSON: routeJSON,
                                waypoints: waypointsArr
                            },
                            success: function (data1) {
    
                            },
                            fail: function(error1) {
                                error1.preventDefault();
                                alert('Adding route to database failed.');
                            }
                        });
                    }
                    // Update the file table, since a new route was added
                    updateFileTable();
                }
            },
            fail: function(error) {
                error.preventDefault();
                alert(error);
            }
        });

        // Reset all the values in the form
        $('#newRouteNumPoints').val(0);
        $('#fileChosen2').val("");
        $('#newRouteName').val("");
        $('.extraPoint').remove();
    });

    // Finding paths between two points
    $('#findPathBetweenForm').submit(function(e){
        e.preventDefault();

        // Hide the table if it is being shown
        $('#pathsFound').attr('hidden', 'true');

        let firstPointLat = this.elements["firstPointLat"].value;
        let firstPointLon = this.elements["firstPointLon"].value;
        let secondPointLat = this.elements["secondPointLat"].value;
        let secondPointLon = this.elements["secondPointLon"].value;
        let deltaVal = this.elements["deltaVal"].value;

        // Remove any error messages
        $('.error').remove();

        let errorCount = 0;

        // Error checking the waypoints again
        if (firstPointLat < -90 || firstPointLat > 90 || !$.isNumeric(firstPointLat)) {
            $('#firstPointLat').after('<small class="error">Must be a number between -90 and 90</small>');
            errorCount++;
        }

        if (firstPointLon < -180 || firstPointLon > 180 || !$.isNumeric(firstPointLon)) {
            $('#firstPointLon').after('<small class="error">Must be a number between -180 and 180</small>');
            errorCount++;
        }

        if (secondPointLat < -90 || secondPointLat > 90 || !$.isNumeric(secondPointLat)) {
            $('#secondPointLat').after('<small class="error">Must be a number between -90 and 90</small>');e
            errorCount++;
        }

        if (secondPointLon < -180 || secondPointLon > 180 || !$.isNumeric(secondPointLon)) {
            $('#secondPointLon').after('<small class="error">Must be a number between -180 and 180</small>');
            errorCount++
        }

        // Delta value has to be positive
        if (deltaVal < 0 || !$.isNumeric(deltaVal)) {
            $('#deltaVal').after('<small class="error">Must be a positive number</small>');
            errorCount++;
        }

        if (errorCount > 0) {
            return;
        }

        // Get the filenames from the dropdown menu
        filenameArray = [];
        $('#fileChosen2 option').each(function(){
            if (this.value != "") {
                filenameArray.push(this.value);
            }
        });

        // Ajax call to get the paths back
        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/findPaths',
            data: {
                filenames: filenameArray,
                lat1: parseFloat(firstPointLat),
                lon1: parseFloat(firstPointLon),
                lat2: parseFloat(secondPointLat),
                lon2: parseFloat(secondPointLon),
                delta: parseFloat(deltaVal)
            },
            success: function(data) {
                let routesArray = data["routes"];
                let tracksArray = data["tracks"];
                // If no paths were found, display an appropriate message
                if (routesArray.length === 0 && tracksArray.length === 0) {
                    $('#pathsFoundTable').html(`<tr>
                                                    <td><i>No paths found</i></td>
                                                </tr>`);
                    $('#pathsFound').removeAttr('hidden');
                    return;
                }
                // Table header
                let table = `
                <thead class="thead-light">
                    <tr>
                        <th class="align-middle">Component</th>
                        <th class="align-middle">Name</th>
                        <th class="align-middle">Number of points</th>
                        <th class="align-middle">Length</th>
                        <th class="align-middle">Loop</th>
                    </tr>
                </thead>`;
                // Routes first
                let i = 1;
                routesArray.forEach(route => {
                    table += '<tr>';
                    table += '<td class="align-middle">Route ' + i + '</td>';
                    let name = '';
                    for (key in route) {
                        if (key == 'loop') {
                            if (route[key] == false) {
                                table += '<td class="align-middle" id="Route'+ key + i +'">FALSE</td>';
                            } else {
                                table += '<td class="align-middle" id="Route'+ key + i +'">TRUE</td>';
                            }
                        } else if (key == 'len') {
                            table += '<td class="align-middle" id="Route'+ key + i +'">' + route[key] + 'm</td>';
                        } else {
                            if (key == 'name') {
                                name = route[key];
                            }
                            table += '<td class="align-middle" id="Route'+ key + i +'">' + route[key] + '</td>';
                        }
                    };
                    table += '</tr>';
                    i += 1;
                });
                // Tracks next
                i = 1;
                tracksArray.forEach(track => {
                    table += '<tr>';
                    table += '<td class="align-middle">Track ' + i + '</td>';
                    let name = '';
                    for (key in track) {
                        if (key == 'loop') {
                            if (track[key] == false) {
                                table += '<td class="align-middle" id="Track'+ key + i +'">FALSE</td>';
                            } else {
                                table += '<td class="align-middle" id="Track'+ key + i +'">TRUE</td>';
                            }
                        } else if (key == 'len') {
                            table += '<td class="align-middle" id="Track'+ key + i +'">' + track[key] + 'm</td>';
                        } else {
                            if (key == 'name') {
                                name = track[key];
                            }
                            table += '<td class="align-middle" id="Track'+ key + i +'">' + track[key] + '</td>';
                        }
                    };
                    table += '</tr>';
                    i += 1;
                });
                // Show the table, and clear the form
                $('#pathsFoundTable').html(table);
                $('#pathsFound').removeAttr('hidden');

                $('#firstPointLat').val("");
                $('#firstPointLon').val("");
                $('#secondPointLat').val("");
                $('#secondPointLon').val("");
                $('#deltaVal').val("");
            },
            fail: function(error) {
                error.preventDefault();
                alert(error);
            }
        });
    });

    // Find how many paths have a specific length
    $('#findPathLengthForm').submit(function(e){
        e.preventDefault();
        let pathLengthToFind = this.elements["pathLengthToFind"].value;

        // Error checking
        if (pathLengthToFind < 0 || !$.isNumeric(pathLengthToFind)) {
            $('#pathLengthToFind').after('<small class="error">Must be a positive number</small>');
            return;
        }

        // Getting filenames again
        filenameArray = [];
        $('#fileChosen2 option').each(function(){
            if (this.value != "") {
                filenameArray.push(this.value);
            }
        });

        $.ajax({
            type: 'get',
            dataType: 'json',
            url: '/findPathsWithLength',
            data: {
                filenames: filenameArray,
                length: pathLengthToFind
            },
            success: function(data) {
                // Display the number of total paths, routes and tracks that matched the length
                $('#numWithSameLength').html('<em>'+data["total"]+' paths found!</em>&nbsp;&nbsp;Number of routes: '+data["totalForRoutes"]+'&nbsp;&nbsp;|&nbsp;&nbsp;Number of tracks: '+data["totalForTracks"]);
                $('#numWithSameLength').removeAttr('hidden');
                $('#pathLengthToFind').val("");
            },
            fail: function(error) {
                error.preventDefault();
                alert(error);
            }
        });
    });

    // Try and login to a database hosted on dursley.socs.uoguelph.ca
    $('#loginDatabase').submit(function(e){
        e.preventDefault();
        
        // Get the username and password and database name from the user input
        username = this.elements["dbUsername"].value;
        password = this.elements["dbPassword"].value;
        name = this.elements["dbName"].value;
        
        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/loginToDatabase',
            data: {
                username: username,
                password: password,
                name: name
            },
            success: function (data) {
                // Remove any connection message that was visible
                $('.connectionMsg').remove();
                // If the connection was successful
                if (data == 'Connection successful!') {
                    loggedIn = 1;
                    // Add a message to show that the attempted connection was successful
                    $('#loginPart').after('<small class="connectionMsg">Connected to database "'+name+'" at '+username+'@dursley.socs.uoguelph.ca</small>');
                    let fileCount = 0;

                    // If there is more than one file on the server, enable the store files button
                    $('#fileChosen2 option').each(function(){
                        if (this.value != "") {
                            fileCount++;
                        }
                    });
                    if (fileCount > 0) {
                        $('#storeFilesButton').removeAttr('disabled');
                    }
                    // Also enable the display status button and the query dropdown, since we are now connected
                    $('#displayStatusButton').removeAttr('disabled');
                    $('#querySelector').removeAttr('disabled');
                    // Ajax call to display status of database right after login
                    $.ajax({
                        type: 'get',
                        dataType: 'text',
                        url: '/displayStatus',
                        data: {
                            username: username,
                            password: password,
                            name: name
                        },
                        success: function (data1) {
                            if (data1.startsWith('Database has')) {
                                alert(data1);
                                let stats = data1.match(/\d+/g);
                                let numFiles = stats[0];
                                let numRoutes = stats[1];
                                let numPoints = stats[2];
                                // If the database's tables are not empty, enable the clear all button
                                if (numFiles > 0 || numRoutes > 0 || numPoints > 0) {
                                    $('#clearAllButton').removeAttr('disabled');
                                }
                            } else {
                                alert(data1 + '\nPlease try again.');
                            }
                        },
                        fail: function (error1) {
                            error1.preventDefault();
                            alert(error1);
                        }
                    });
                } else if (data == 'Connection failed!') {
                    // Connection failed, so disable all buttons and show an appropriate alert
                    loggedIn = 0;
                    $('#storeFilesButton').attr('disabled', true);
                    $('#clearAllButton').attr('disabled', true);
                    $('#displayStatusButton').attr('disabled', true);
                    $('#querySelector').attr('disabled', true);
                    alert(data + '\nPlease try again.');
                }
            },
            fail: function (error) {
                error.preventDefault();
                alert(error);
            }
        });

    });

    // Store files on the server into the database
    $('#storeFilesButton').click(function(e){
        e.preventDefault();

        filenameArray = [];
        $('#fileChosen2 option').each(function(){
            if (this.value != "") {
                filenameArray.push(this.value);
            }
        });

        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/storeFiles',
            data: {
                username: username,
                password: password,
                name: name,
                fileList: filenameArray
            },
            success: function (data) {
                if (data == 'Added successfully!') {
                    $.ajax({
                        type: 'get',
                        dataType: 'text',
                        url: '/displayStatus',
                        data: {
                            username: username,
                            password: password,
                            name: name
                        },
                        success: function (data1) {
                            if (data1.startsWith('Database has')) {
                                alert(data1);
                                let stats = data1.match(/\d+/g);
                                let numFiles = stats[0];
                                let numRoutes = stats[1];
                                let numPoints = stats[2];
                                // After storing the files, check if it was successful then enable clear all button, since there are now records in the table
                                if (numFiles > 0 || numRoutes > 0 || numPoints > 0) {
                                    $('#clearAllButton').removeAttr('disabled');
                                }
                            } else {
                                alert(data1 + '\nPlease try again.');
                            }
                        },
                        fail: function (error1) {
                            error1.preventDefault();
                            alert(error1);
                        }
                    });
                } else {
                    alert(data + '\nPlease try again.');
                }
            },
            fail: function (error1) {
                error1.preventDefault();
                alert(error1);
            }
        });
    });

    // Clear all records in the database
    $('#clearAllButton').click(function(e){
        e.preventDefault();

        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/clearAllRows',
            data: {
                username: username,
                password: password,
                name: name
            },
            success: function (data) {
                if (data == 'Cleared successfully!') {
                    // If successful, then display status again (which should show 0 for all)
                    $.ajax({
                        type: 'get',
                        dataType: 'text',
                        url: '/displayStatus',
                        data: {
                            username: username,
                            password: password,
                            name: name
                        },
                        success: function (data1) {
                            if (data1.startsWith('Database has')) {
                                alert(data1);
                            } else {
                                alert(data1 + '\nPlease try again.');
                            }
                        },
                        fail: function (error1) {
                            error1.preventDefault();
                            alert(error1);
                        }
                    });
                    // Disable the button again, since all records have been cleared
                    $('#clearAllButton').attr('disabled', true);
                } else {
                    alert(data + '\nPlease try again.');
                }
            },
            fail: function (error1) {
                error1.preventDefault();
                alert(error1);
            }
        });
    });

    // Display status of the database (number of records in each table)
    $('#displayStatusButton').click(function(e){
        e.preventDefault();
        $.ajax({
            type: 'get',
            dataType: 'text',
            url: '/displayStatus',
            data: {
                username: username,
                password: password,
                name: name
            },
            success: function (data) {
                if (data.startsWith('Database has')) {
                    alert(data);
                    let stats = data.match(/\d+/g);
                    let numFiles = stats[0];
                    let numRoutes = stats[1];
                    let numPoints = stats[2];
                    if (numFiles > 0 || numRoutes > 0 || numPoints > 0) {
                        $('#clearAllButton').removeAttr('disabled');
                    }
                } else {
                    alert(data + '\nPlease try again.');
                }
            },
            fail: function (error) {
                error.preventDefault();
                alert(error);
            }
        });
    });

    // When a database query is executed, same process for all, just different endpoints and information for each
    $('#executeQueries').submit(function(e){
        e.preventDefault();
        let endpoint;
        // Find which query was chosen
        let query = $('#queryRunner').find(":selected").text();
        if (query == 'Display all routes' || query == 'Display all routes from specific file') {
            // Get all the choices like if it is sorted, sort order and which file to choose from
            let isSorted = 'false';
            let sortType = $('input[type="radio"][name="sortOptions"]:checked').val();
            if (sortType != undefined) {
                isSorted = 'true';
            }
            let order = $('input[type="radio"][name="sortOrder"]:checked').val();
            let file = $('#filesInDB').find(":selected").text();
            // Set the appropriate endpoint depending on query
            if (query == 'Display all routes') {
                endpoint = '/displayAllRoutes';
            } else {
                endpoint = '/displaySpecificRoutes';
            }
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: endpoint,
                data: {
                    username: username,
                    password: password,
                    name: name,
                    sortType: sortType,
                    order: order,
                    isSorted: isSorted,
                    filename: file
                },
                success: function(data) {
                    if (!Array.isArray(data)) {
                        alert('Could not execute query!');
                        return;
                    }
                    // If the query returned something
                    if (data.length > 0) {
                        // Set table headers
                        let table = `
                        <thead class="thead-light">
                            <tr>
                                <th class="align-middle">Route ID</th>
                                <th class="align-middle">Route name</th>
                                <th class="align-middle">Length</th>
                                <th class="align-middle">File origin</th>
                            </tr>
                        </thead>`;
                        // Add all the routes
                        let i = 1;
                        for (let route of data) {
                            table += '<tr>';
                            table += '<td class="align-middle">' + route.route_id + '</td>';
                            if (route.route_name == null || route.route_name == 'None') {
                                table += '<td class="align-middle">Unnamed route '+i+'</td>';
                                i++;
                            } else {
                                table += '<td class="align-middle">' + route.route_name + '</td>';
                            }
                            table += '<td class="align-middle">' + route.route_len + '</td>';
                            table += '<td class="align-middle">' + route.file_name + '</td></tr>';
                        }
                        $('#queryResultsTable').html(table);
                    } else {
                        // If there were no results, display appropriate message
                        let table = `
                        <tr>
                            <td><i>No results</i></td>
                        </tr>`;
                        $('#queryResultsTable').html(table);
                    }
                },
                fail: function(error) {
                    error.preventDefault();
                    alert(error);
                }
            });
        } else if (query == 'Display all points from a route' || query == 'Display all points from specific file') {
            let isSorted = 'false';
            let sortType = $('input[type="radio"][name="sortOptions"]:checked').val();
            if (sortType != undefined) {
                isSorted = 'true';
            }
            let order = $('input[type="radio"][name="sortOrder"]:checked').val();
            let file = $('#filesInDB').find(":selected").text();
            let routeID = $('#routesInDB').find(":selected").text();
            if (query == 'Display all points from a route') {
                endpoint = '/displayAllPoints';
            } else {
                endpoint = '/displaySpecificPoints';
            }
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: endpoint,
                data: {
                    username: username,
                    password: password,
                    name: name,
                    routeID: routeID,
                    sortType: sortType,
                    order: order,
                    isSorted: isSorted,
                    filename: file
                },
                success: function(data) {
                    if (!Array.isArray(data)) {
                        alert('Could not execute query!');
                        return;
                    }
                    if (data.length > 0) {
                        let table = `
                        <thead class="thead-light">
                            <tr>
                                <th class="align-middle">Point ID</th>
                                <th class="align-middle">Point name</th>
                                <th class="align-middle">Latitude</th>
                                <th class="align-middle">Longitude</th>
                                <th class="align-middle">Index in route</th>
                            </tr>
                        </thead>`;
                        let i = 1;
                        for (let point of data) {
                            table += '<tr>';
                            table += '<td class="align-middle">' + point.point_id + '</td>';
                            if (point.point_name == null) {
                                table += '<td class="align-middle">Unnamed point '+i+'</td>';
                                i++;
                            } else {
                                table += '<td class="align-middle">' + point.point_name + '</td>';
                            }
                            table += '<td class="align-middle">' + point.latitude + '</td>';
                            table += '<td class="align-middle">' + point.longitude + '</td>';
                            table += '<td class="align-middle">' + point.point_index + '</td></tr>';
                        }
                        $('#queryResultsTable').html(table);
                    } else {
                        let table = `
                        <tr>
                            <td><i>No results</i></td>
                        </tr>`;
                        $('#queryResultsTable').html(table);
                    }
                },
                fail: function(error) {
                    error.preventDefault();
                    alert(error);
                }
            });
        } else if (query == 'Display shortest/longest routes from specific file') {
            let isSorted = 'false';
            let sortType = $('input[type="radio"][name="sortOptions"]:checked').val();
            if (sortType != undefined) {
                isSorted = 'true';
            }
            let order = $('input[type="radio"][name="sortOrder"]:checked').val();
            let file = $('#filesInDB').find(":selected").text();
            let numRecords = $('#numRoutesToShow').val();
            let type = $('input[type="radio"][name="shortOrLong"]:checked').val();
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: '/getNRoutesFromFile',
                data: {
                    username: username,
                    password: password,
                    name: name,
                    sortType: sortType,
                    order: order,
                    isSorted: isSorted,
                    filename: file,
                    numRecords: numRecords,
                    type: type
                },
                success: function(data) {
                    if (!Array.isArray(data)) {
                        alert('Could not execute query!');
                        return;
                    }
                    if (data.length > 0) {
                        let table = `
                        <thead class="thead-light">
                            <tr>
                                <th class="align-middle">Route ID</th>
                                <th class="align-middle">Route name</th>
                                <th class="align-middle">Length</th>
                                <th class="align-middle">File origin</th>
                            </tr>
                        </thead>`;
                        let i = 1;
                        for (let route of data) {
                            table += '<tr>';
                            table += '<td class="align-middle">' + route.route_id + '</td>';
                            if (route.route_name == null || route.route_name == 'None') {
                                table += '<td class="align-middle">Unnamed route '+i+'</td>';
                                i++;
                            } else {
                                table += '<td class="align-middle">' + route.route_name + '</td>';
                            }
                            table += '<td class="align-middle">' + route.route_len + '</td>';
                            table += '<td class="align-middle">' + route.file_name + '</td></tr>';
                        }
                        $('#queryResultsTable').html(table);
                    } else {
                        let table = `
                        <tr>
                            <td><i>No results</i></td>
                        </tr>`;
                        $('#queryResultsTable').html(table);
                    }
                },
                fail: function(error) {
                    error.preventDefault();
                    alert(error);
                }
            });
        }
    });

    // When a query is chosen from the dropdown, change the available options and visible buttons/choices, again repeated code, but differs by what options to show/hide and endpoints
    $('#querySelector').change(function(e) {
        e.preventDefault();
        let query = this.value;
        $('#queryResultsTable').html('');
        if (query == 'Display all routes') {
            // Remove these options
            $('#sortByRadio').removeAttr('hidden');
            $('#fileChosen3').attr('hidden', true);
            $('#routeChosen').attr('hidden', true);
            $('#shortOrLongRadio').attr('hidden', true);
            $('#numRecordsToShow').attr('hidden', true);
        } else if (query == 'Display all routes from specific file' || query == 'Display all points from specific file') {
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: '/listFilesInDatabase',
                data: {
                    username: username,
                    password: password,
                    name: name
                },
                success: function (data1) {
                    if (data1.length > 0) {
                        for (let file of data1) {
                            let exists = false;
                            $('#filesInDB option').each(function(){
                                if (this.value == file.file_name) {
                                    exists = true;
                                    return false;
                                }
                            });
                            if (exists == false) {
                                $('#filesInDB').append('<option value="'+file.file_name+'">'+file.file_name+'</option>');
                            }
                        }
                    }
                },
                fail: function(error1) {
                    error1.preventDefault();
                    alert(error1);
                }
            });
            $('#fileChosen3').removeAttr('hidden');
            $('#sortByRadio').removeAttr('hidden');
            $('#routeChosen').attr('hidden', true);
            $('#shortOrLongRadio').attr('hidden', true);
            $('#numRecordsToShow').attr('hidden', true);
        } else if (query == 'Display all points from a route') {
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: '/listRteIDs',
                data: {
                    username: username,
                    password: password,
                    name: name
                },
                success: function (data1) {
                    if (data1.length > 0) {
                        for (let route of data1) {
                            let exists = false;
                            $('#routesInDB option').each(function(){
                                if (this.value == route.route_id) {
                                    exists = true;
                                    return false;
                                }
                            });
                            if (exists == false) {
                                $('#routesInDB').append('<option value="'+route.route_id+'">'+route.route_id+'</option>');
                            }
                        }
                    }
                },
                fail: function(error1) {
                    error1.preventDefault();
                    alert(error1);
                }
            });
            $('#routeChosen').removeAttr('hidden');
            $('#fileChosen3').attr('hidden', true);
            $('#sortByRadio').attr('hidden', true);
            $('#shortOrLongRadio').attr('hidden', true);
            $('#numRecordsToShow').attr('hidden', true);
        } else if (query == 'Display shortest/longest routes from specific file') {
            $.ajax({
                type: 'get',
                dataType: 'json',
                url: '/listFilesInDatabase',
                data: {
                    username: username,
                    password: password,
                    name: name
                },
                success: function (data1) {
                    if (data1.length > 0) {
                        for (let file of data1) {
                            let exists = false;
                            $('#filesInDB option').each(function(){
                                if (this.value == file.file_name) {
                                    exists = true;
                                    return false;
                                }
                            });
                            if (exists == false) {
                                $('#filesInDB').append('<option value="'+file.file_name+'">'+file.file_name+'</option>');
                            }
                        }
                    }
                },
                fail: function(error1) {
                    error1.preventDefault();
                    alert(error1);
                }
            });
            $('#fileChosen3').removeAttr('hidden');
            $('#sortByRadio').removeAttr('hidden');
            $('#shortOrLongRadio').removeAttr('hidden');
            $('#numRecordsToShow').removeAttr('hidden');
            $('#routeChosen').attr('hidden', true);
        }
        $('#executeButton').removeAttr('hidden');
    });
});
