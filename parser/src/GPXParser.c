#include "GPXParser.h"
#include "GPXHelpers.h"
#include "LinkedListAPI.h"

/** Function to create an GPX object based on the contents of an GPX file.
 *@pre File name cannot be an empty string or NULL.
       File represented by this name must exist and must be readable.
 *@post Either:
        A valid GPXdoc has been created and its address was returned
		or 
		An error occurred, and NULL was returned
 *@return the pinter to the new struct or NULL
 *@param fileName - a string containing the name of the GPX file
**/
GPXdoc *createGPXdoc(char* fileName) {

    // Initialize a xmlDoc variable
    xmlDoc *doc = NULL;

    // If the user enters no filename, return NULL
    if (fileName == NULL) {
        return NULL;
    }

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     * 
     * Taken from the sample code provided.
     */
    LIBXML_TEST_VERSION

    // Calls the xmlReadFile function to get a parse-able tree
    doc = xmlReadFile(fileName, NULL, 0);

    // If the function failed for any reason, it will return NULL
    if (doc == NULL) {

        // Free doc and cleanup any variables that could have been used by the XML parser
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return NULL;

    }

    // Initialize a new xmlNode to the root element of the returned tree
    xmlNode *root_node = xmlDocGetRootElement(doc);

    // If the tree is empty, or if it is not a gpx file
    if (root_node == NULL || strcmp((const char *) root_node->name, "gpx") != 0) {

        // Free doc and cleanup any variables used by the parser
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return NULL;

    }

    // Malloc space for a new GPXdoc struct
    GPXdoc *newDoc = malloc(sizeof(GPXdoc));
    
    // Checking if namespace exists and is not an empty string (which makes the file invalid)
    if (root_node->ns == NULL || root_node->ns->href == NULL || strcmp((const char *)root_node->ns->href, "") == 0) {

        // Freeing
        xmlFreeDoc(doc);
        xmlCleanupParser();
        free(newDoc);

        return NULL;

    }

    // Otherwise, copy namespace into the struct
    strcpy(newDoc->namespace, (const char *) root_node->ns->href);

    // Flag variables to keep track of if the doc has a version and creator
    int versionCheck = 0, creatorCheck = 0;

    // Declaring an attribute iterator
    xmlAttr *attr;

    // Looping through all the attributes of the root node, until NULL is hit
    for (attr = root_node->properties; attr != NULL; attr = attr->next) {

        // Setting the value, name and content variables
        xmlNode *value = attr->children;
        char *attrName = (char *)attr->name;
        char *cont = (char *)(value->content);

        // If the version attribute is found
        if (strcmp(attrName, "version") == 0) {

            // Version flag is set to 1
            versionCheck++;

            // Copy the double value of content into the struct, using atof
            newDoc->version = atof(cont);

        } else if (strcmp(attrName, "creator") == 0) { // If the creator attribute is found

            // Creator flag is set to 1
            creatorCheck++;

            // Mallocing enough space for creator, and then copying it into the struct
            newDoc->creator = malloc(strlen(cont) + 1);
            strcpy(newDoc->creator, cont);

        }
    }

    // If no creator attribute
    if (creatorCheck == 0) {

        // Freeing
        xmlFreeDoc(doc);
        xmlCleanupParser();
        free(newDoc);

        return NULL;

    } else if (versionCheck == 0) { // If no version attribute

        // Freeing
        xmlFreeDoc(doc);
        xmlCleanupParser();
        free(newDoc->creator); // Free creator too
        free(newDoc);

        return NULL;

    }

    // Initialize all the lists in the struct, because they can't be NULL
    newDoc->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    newDoc->routes = initializeList(&routeToString, &deleteRoute, &compareRoutes);
    newDoc->tracks = initializeList(&trackToString, &deleteTrack, &compareTracks);

    // Call recursiveReader to input all the other information into the doc
    recursiveReader(root_node, newDoc);

    // Freeing the tree (since we have a parsed struct now) and cleanup any variables used/allocated by the parser
    xmlFreeDoc(doc);
    xmlCleanupParser();

    // Return a pointer to the new GPXDoc struct, so we can change it later on
    return newDoc;

}

/** Function to create a string representation of an GPX object.
 *@pre GPX object exists, is not null, and is valid
 *@post GPX has not been modified in any way, and a string representing the GPX contents has been created
 *@return a string contaning a humanly readable representation of an GPX object
 *@param obj - a pointer to an GPX struct
**/
char *GPXdocToString(GPXdoc* doc) {

    // If the doc is NULL, return NULL
    if (doc == NULL) {
        return NULL;
    }

    // Call toString on each of the lists
    char *waypoints = toString(doc->waypoints);
    char *routes = toString(doc->routes);
    char *tracks = toString(doc->tracks);


    // To avoid any undefined behavior if the strings are empty

    if (waypoints[0] == '\0') {
        
        // Free the old pointer
        free(waypoints);

        // Malloc enough space for the string "NO WAYPOINTS!" with null terminator, and then copy it
        waypoints = malloc(14);
        strcpy(waypoints, "NO WAYPOINTS!");

    }

    if (routes[0] == '\0') {

        // Free the old pointer
        free(routes);

        // Malloc enough space for the string "NO ROUTES!" with null terminator, and then copy it
        routes = malloc(11);
        strcpy(routes, "NO ROUTES!");

    }

    if (tracks[0] == '\0') {

        // Free the old pointer
        free(tracks);

        // Malloc enough space for the string "NO TRACKS!" with null terminator, and then copy it
        tracks = malloc(11);
        strcpy(tracks, "NO TRACKS!");

    }
    
    // Otherwise malloc enough space for the complete string, which is the length of all the substrings + the length of labels
    int length = strlen(doc->namespace) + strlen(doc->creator) + strlen(waypoints) + strlen(routes) + strlen(tracks) + 43;
    char *tmpStr = malloc(length);

    // If the malloc failed
    if (tmpStr == NULL) {

        free(waypoints);
        free(routes);
        free(tracks);
        free(tmpStr);

        return NULL;

    }

    // Copy the new string into the return string using sprintf
    sprintf(tmpStr, "\nnamespace: %s\nversion: %g\ncreator: %s\n\n%s\n\n%s\n\n%s\n", doc->namespace, doc->version, doc->creator, waypoints, routes, tracks);

    // Free the strings retrieved from the toString calls to prevent memory leaks
    free(waypoints);
    free(routes);
    free(tracks);

    // Return the new string
    return tmpStr;

}

/** Function to delete doc content and free all the memory.
 *@pre GPX object exists, is not null, and has not been freed
 *@post GPX object had been freed
 *@return none
 *@param obj - a pointer to an GPX struct
**/
void deleteGPXdoc(GPXdoc* doc) {

    // If the doc is empty, nothing to free, so just return
    if (doc == NULL) {
        return;
    }


    free(doc->creator);

    freeList(doc->waypoints);
    freeList(doc->routes);
    freeList(doc->tracks);

    free(doc);

}


/* For the five "get..." functions below, return the count of specified entities from the file.  
They all share the same format and only differ in what they have to count.
 
 *@pre GPX object exists, is not null, and has not been freed
 *@post GPX object has not been modified in any way
 *@return the number of entities in the GPXdoc object
 *@param obj - a pointer to an GPXdoc struct
 */


// Total number of waypoints in the GPX file
int getNumWaypoints(const GPXdoc* doc) {

    // If the doc is NULL, just return 0
    if (doc == NULL) {
        return 0;
    }

    // Otherwise call the List API function to return the length of the list
    return getLength(doc->waypoints);

}

//Total number of routes in the GPX file
int getNumRoutes(const GPXdoc* doc) {

    // If the doc is NULL, just return 0
    if (doc == NULL) {
        return 0;
    }

    // Otherwise call the List API function to return the length of the list
    return getLength(doc->routes);

}

//Total number of tracks in the GPX file
int getNumTracks(const GPXdoc* doc) {

    // If the doc is NULL, just return 0
    if (doc == NULL) {
        return 0;
    }

    // Otherwise call the List API function to return the length of the list
    return getLength(doc->tracks);

}

//Total number of segments in all tracks in the document
int getNumSegments(const GPXdoc* doc) {

    // If the doc is NULL, just return 0
    if (doc == NULL) {
        return 0;
    }

    // Initialize count to 0
    int count = 0;

    void *elem; // Variable to hold the data
    List *list = doc->tracks; // Initialize new list variable, so we dont impact the original
    ListIterator iter = createIterator(list);// Create a new iterator 

    // Loop through each item in the list
	while ((elem = nextElement(&iter)) != NULL) {

        // Casting to Track
        Track *tmpTrk = (Track *)elem;

        // Add to the count the length of each segments list
        count += getLength(tmpTrk->segments);
	}

    // Return total count
    return count;

}

//Total number of GPXData elements in the document
int getNumGPXData(const GPXdoc* doc) {

    // If the doc is NULL, just return 0
    if (doc == NULL) {
        return 0;
    }

    // Initialize count to 0
    int count = 0;
    
    // Same steps as for getNumSegments to get total number of otherData's in the waypoints list
    void *elem;
    List *list = doc->waypoints;
    ListIterator iter = createIterator(list);

	while ((elem = nextElement(&iter)) != NULL) {
        Waypoint *tmpWpt = (Waypoint *)elem;

        // Also adding the counts of non-empty names
        if (tmpWpt->name[0] != '\0') {
            count++;
        }

        // Add the length of otherData
        count += getLength(tmpWpt->otherData);
	}

    // Change list to iterate to routes
    list = doc->routes;
    iter = createIterator(list);

    // Same steps as before, except with another level of nesting, because there is a list of waypoints inside the list of routes
	while ((elem = nextElement(&iter)) != NULL) {

        // Casting to Route
        Route *tmpRte = (Route *)elem;

        // Initialize 3 new variables as to not overwrite the previous ones
        void *elem2;
        List *tmpList1 = tmpRte->waypoints;
        ListIterator tmpIter1 = createIterator(tmpList1);

        // Iterate through until NULL is hit
        while ((elem2 = nextElement(&tmpIter1)) != NULL) {

            // Casting to Waypoint for each element in the waypoints list
            Waypoint *tmpWpt2 = (Waypoint *)elem2;

            // Adding the non-empty waypoint names to the count
            if (tmpWpt2->name[0] != '\0') {
                count++;
            }

        }

        // Adding the non-empty route names to the count
        if (tmpRte->name[0] != '\0') {
            count++;
        }

        // Add the length of otherData
        count += getLength(tmpRte->otherData);
	}

    // Change the list to iterate to tracks
    list = doc->tracks;
    iter = createIterator(list);

    // Loop through the list of tracks until NULL is hit
	while ((elem = nextElement(&iter)) != NULL) {

        // Casting to Track
        Track *tmpTrk = (Track *)elem;

        // Initialize 3 new variables as to not overwrite the previous ones
        void *elem3;
        List *tmpList2 = tmpTrk->segments;
        ListIterator tmpIter2 = createIterator(tmpList2);

        // Iterate through the segments list
        while ((elem3 = nextElement(&tmpIter2)) != NULL) {

            // Casting to TrackSegment
            TrackSegment *tmpTrkSeg = (TrackSegment *)elem3;

            // Initialize 3 new variables as to not overwrite the previous ones
            void *elem4;
            List *tmpList3 = tmpTrkSeg->waypoints;
            ListIterator tmpIter3 = createIterator(tmpList3);

            // Loop through the waypoints list for each segment
            while ((elem4 = nextElement(&tmpIter3)) != NULL) {

                // Casting to Waypoint
                Waypoint *tmpWpt3 = (Waypoint *)elem4;

                // Adding all the non-empty track point names
                if (tmpWpt3->name[0] != '\0') {
                    count++;
                }

                // Adding the length of the otherData list
                count += getLength(tmpWpt3->otherData);

            }

        }

        // Adding all the non-empty track names
        if (tmpTrk->name[0] != '\0') {
            count++;
        }

        // Adding the length of the otherData
        count += getLength(tmpTrk->otherData);

	}

    // Return the total count
    return count;

}

// Function that returns a waypoint with the given name.  If more than one exists, return the first one.  
// Return NULL if the waypoint does not exist
Waypoint *getWaypoint(const GPXdoc* doc, char* name) {

    // If the doc is NULL, or the name is NULL, return NULL
    if (doc == NULL || name == NULL) {
        return NULL;
    }

    // Iterate through the waypoints list
    void *elem;
    List *list = doc->waypoints;
    ListIterator iter = createIterator(list);

	while ((elem = nextElement(&iter)) != NULL) {

        // Cast to Waypoint
        Waypoint *tmpWpt = (Waypoint *)elem;

        // If a match with the name is found, return the pointer to that element
        if (strcmp(tmpWpt->name, name) == 0) {
            return tmpWpt;
        }
	}

    return NULL;

}

// Function that returns a track with the given name.  If more than one exists, return the first one. 
// Return NULL if the track does not exist 
Track *getTrack(const GPXdoc* doc, char* name) {

    // If the doc is NULL, or the name is NULL, return NULL
    if (doc == NULL || name == NULL) {
        return NULL;
    }

    // Iterate through the routes list
    void *elem;
    List *list = doc->tracks;
    ListIterator iter = createIterator(list);

	while ((elem = nextElement(&iter)) != NULL) {

        // Cast to Track
        Track *tmpTrk = (Track *)elem;

        // If a match with the name is found, return the pointer to that element
        if (strcmp(tmpTrk->name, name) == 0) {
            return tmpTrk;
        }
	}

    return NULL;

}

// Function that returns a route with the given name.  If more than one exists, return the first one.  
// Return NULL if the route does not exist
Route *getRoute(const GPXdoc* doc, char* name) {

    // If the doc is NULL, or the name is NULL, return NULL
    if (doc == NULL || name == NULL) {
        return NULL;
    }

    // Iterate through the tracks list
    void *elem;
    List *list = doc->routes;
    ListIterator iter = createIterator(list);

	while ((elem = nextElement(&iter)) != NULL) {

        // Cast to Route
        Route *tmpRte = (Route *)elem;

        // If a match with the name is found, return the pointer to that element
        if (strcmp(tmpRte->name, name) == 0) {
            return tmpRte;
        }
	}

    return NULL;

}


/************************************* List helper functions *************************************
 *                  ALL OF THE BELOW FUNCTIONS ARE IMPLEMENTED IN A SIMILAR WAY                  *
 *                                                                                               *
 *           THE DELETE FUNCTIONS CHECK IF THE DATA IS NULL, IF YES THEY SIMPLY RETURN           *
 *       OTHERWISE THEY DELETE EACH ELEMENT INSIDE THAT STRUCT BY FREEING IT (IF POSSIBLE)       *
 *                                                                                               *
 *                    THE *TOSTRING FUNCTIONS RETURN NULL IF THE DATA IS NULL                    *
 *              OTHERWISE THEY RETURN A STRING OF EVERY ELEMENT CONCATENATED AS ONE              *
 *                                                                                               *
 *    THE COMPARE FUNCTIONS ARE JUST A STUB BECAUSE THEY WILL NOT BE USED, AND JUST RETURN 0     *
 *                                                                                               *
 *************************************************************************************************/

void deleteGpxData(void *data) {

    if (data == NULL) {
        return;
    }

    GPXData *tmpData = (GPXData *)data;
    free(tmpData);

}
char *gpxDataToString(void *data) {

    if (data == NULL) {
        return NULL;
    }

    GPXData *tmpData = (GPXData *)data;
    
    int length = strlen(tmpData->name) + strlen(tmpData->value) + 6;
    char *tmpStr = malloc(length);
    if (tmpStr == NULL) {
        return NULL;
    }
    sprintf(tmpStr, "\t\t%s: %s\n", tmpData->name, tmpData->value);
    return tmpStr;
    
}
int compareGpxData(const void *first, const void *second) { return 0; }

void deleteWaypoint(void *data) {

    if (data == NULL) {
        return;
    }

    Waypoint *tmpWpt = (Waypoint *)data;
    if (tmpWpt->name != NULL) {
        free(tmpWpt->name);
    }
    
    if (tmpWpt->otherData != NULL) {
        freeList(tmpWpt->otherData);
    }

    free(tmpWpt);

}
char *waypointToString(void *data) {
    
    if (data == NULL) {
        return NULL;
    }

    Waypoint *tmpWpt = (Waypoint *)data;
    char *other = toString(tmpWpt->otherData);
    
    int length = strlen(tmpWpt->name) + strlen(other) + 76;

    char *tmpStr = malloc(length);
    if (tmpStr == NULL) {
        return NULL;
    }
    
    sprintf(tmpStr, "\tWAYPOINT\n\t\tname: %s | lat: %f | lon: %f\n%s", tmpWpt->name, tmpWpt->latitude, tmpWpt->longitude, other);
    free(other);

    return tmpStr;

}
int compareWaypoints(const void *first, const void *second) { return 0; }

void deleteRoute(void *data) {

    if (data == NULL) {
        return;
    }

    Route *tmpRte = (Route *)data;
    free(tmpRte->name);
    freeList(tmpRte->waypoints);
    freeList(tmpRte->otherData);
    free(tmpRte);

}
char *routeToString(void *data) {

    if (data == NULL) {
        return NULL;
    }

    Route *tmpRte = (Route *)data;

    char *waypoints = toString(tmpRte->waypoints);
    char *other = toString(tmpRte->otherData);

    int length = strlen(tmpRte->name) + strlen(waypoints) + strlen(other) + 18;
    char *tmpStr = malloc(length);
    if (tmpStr == NULL) {
        return NULL;
    }
    
    sprintf(tmpStr, "\tROUTE\n\t\tname: %s\n%s\n%s", tmpRte->name, waypoints, other);
    free(waypoints);
    free(other);

    return tmpStr;
    

}
int compareRoutes(const void *first, const void *second) { return 0; }

void deleteTrackSegment(void *data) {

    if (data == NULL) {
        return;
    }

    TrackSegment *tmpTrkSeg = (TrackSegment *)data;
    freeList(tmpTrkSeg->waypoints);
    free(tmpTrkSeg);

}
char *trackSegmentToString(void *data) {
    
    if (data == NULL) {
        return NULL;
    }

    TrackSegment *tmpTrkSeg = (TrackSegment *)data;
    char *waypoints = toString(tmpTrkSeg->waypoints);

    int length = strlen(waypoints) + 16;

    char *tmpStr = malloc(length);
    if (tmpStr == NULL) {
        return NULL;
    }
    
    sprintf(tmpStr, "\tTRACK SEGMENT\n%s", waypoints);
    free(waypoints);

    return tmpStr;

}
int compareTrackSegments(const void *first, const void *second) { return 0; }

void deleteTrack(void *data) {

    if (data == NULL) {
        return;
    }

    Track *tmpTrk = (Track *)data;
    free(tmpTrk->name);
    freeList(tmpTrk->segments);
    freeList(tmpTrk->otherData);
    free(tmpTrk);

}
char *trackToString(void *data) {

    if (data == NULL) {
        return NULL;
    }

    Track *tmpTrk = (Track *)data;
    char *segments = toString(tmpTrk->segments);
    char *other = toString(tmpTrk->otherData);

    int length = strlen(tmpTrk->name) + strlen(segments) + strlen(other) + 20;

    char *tmpStr = malloc(length);
    if (tmpStr == NULL) {
        return NULL;
    }
    
    sprintf(tmpStr, "\tTRACK\n\t\tname: %s\n\n%s\n\n%s", tmpTrk->name, segments, other);
    free(segments);
    free(other);

    return tmpStr;

}
int compareTracks(const void *first, const void *second) { return 0; }

// Create a GPXdoc struct if a valid file is provided, validated by a schema file
GPXdoc *createValidGPXdoc(char* fileName, char *gpxSchemaFile) {

    // Initialize a xmlDoc variable
    xmlDoc *doc = NULL;

    // If the user enters no filename, return NULL
    if (fileName == NULL || gpxSchemaFile == NULL) {
        return NULL;
    }

    /*
     * this initialize the library and check potential ABI mismatches
     * between the version it was compiled for and the actual shared
     * library used.
     * 
     * Taken from the sample code provided.
     */
    LIBXML_TEST_VERSION

    // Calls the xmlReadFile function to get a parse-able tree
    doc = xmlReadFile(fileName, NULL, 0);

    // If the function failed for any reason, it will return NULL
    if (doc == NULL) {

        // Free doc and cleanup any variables that could have been used by the XML parser
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return NULL;

    }

    xmlSchema *schema = NULL;

    xmlLineNumbersDefault(1);

    xmlSchemaParserCtxt *newCtxt = xmlSchemaNewParserCtxt(gpxSchemaFile);

    schema = xmlSchemaParse(newCtxt);
    xmlSchemaFreeParserCtxt(newCtxt);

    xmlSchemaValidCtxt *ctxt = xmlSchemaNewValidCtxt(schema);
    int ret = xmlSchemaValidateDoc(ctxt, doc);
    xmlSchemaFreeValidCtxt(ctxt);
    xmlSchemaFree(schema);
    xmlSchemaCleanupTypes();

    if (ret != 0) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    // Initialize a new xmlNode to the root element of the returned tree
    xmlNode *root_node = xmlDocGetRootElement(doc);

    // If the tree is empty, or if it is not a gpx file
    if (root_node == NULL || strcmp((const char *) root_node->name, "gpx") != 0) {

        // Free doc and cleanup any variables used by the parser
        xmlFreeDoc(doc);
        xmlCleanupParser();

        return NULL;

    }

    // Malloc space for a new GPXdoc struct
    GPXdoc *newDoc = malloc(sizeof(GPXdoc));
    
    // Checking if namespace exists and is not an empty string (which makes the file invalid)
    if (root_node->ns == NULL || root_node->ns->href == NULL || strcmp((const char *)root_node->ns->href, "") == 0) {

        // Freeing
        xmlFreeDoc(doc);
        xmlCleanupParser();
        free(newDoc);

        return NULL;

    }

    // Otherwise, copy namespace into the struct
    strcpy(newDoc->namespace, (const char *) root_node->ns->href);

    // Flag variables to keep track of if the doc has a version and creator
    int versionCheck = 0, creatorCheck = 0;

    // Declaring an attribute iterator
    xmlAttr *attr;

    // Looping through all the attributes of the root node, until NULL is hit
    for (attr = root_node->properties; attr != NULL; attr = attr->next) {

        // Setting the value, name and content variables
        xmlNode *value = attr->children;
        char *attrName = (char *)attr->name;
        char *cont = (char *)(value->content);

        // If the version attribute is found
        if (strcmp(attrName, "version") == 0) {

            // Version flag is set to 1
            versionCheck++;

            // Copy the double value of content into the struct, using atof
            newDoc->version = atof(cont);

        } else if (strcmp(attrName, "creator") == 0) { // If the creator attribute is found

            // Creator flag is set to 1
            creatorCheck++;

            // Mallocing enough space for creator, and then copying it into the struct
            newDoc->creator = malloc(strlen(cont) + 1);
            strcpy(newDoc->creator, cont);

        }
    }

    // If no creator attribute
    if (creatorCheck == 0) {

        // Freeing
        xmlFreeDoc(doc);
        xmlCleanupParser();
        free(newDoc);

        return NULL;

    } else if (versionCheck == 0) { // If no version attribute

        // Freeing
        xmlFreeDoc(doc);
        xmlCleanupParser();
        free(newDoc->creator); // Free creator too
        free(newDoc);

        return NULL;

    }

    // Initialize all the lists in the struct, because they can't be NULL
    newDoc->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    newDoc->routes = initializeList(&routeToString, &deleteRoute, &compareRoutes);
    newDoc->tracks = initializeList(&trackToString, &deleteTrack, &compareTracks);

    // Call recursiveReader to input all the other information into the doc
    recursiveReader(root_node, newDoc);

    // Freeing the tree (since we have a parsed struct now) and cleanup any variables used/allocated by the parser
    xmlFreeDoc(doc);
    xmlCleanupParser();

    // Return a pointer to the new GPXDoc struct, so we can change it later on
    return newDoc;

}

// Validate a GPXdoc struct
bool validateGPXDoc(GPXdoc *doc, char *gpxSchemaFile) {

    // If either parameter is NULL, return false
    if (doc == NULL || gpxSchemaFile == NULL) {
        return false;
    }

    // If an invalid schema file is provided, return false
    if (strlen(gpxSchemaFile) == 0) {
        return false;
    } else if (gpxSchemaFile[0] == ' ') {
        return false;
    }

    // Try and convert the GPXdoc struct to an XML tree, if it fails, return false
    xmlDoc *tmpDoc = gpxDocToXMLDoc(doc);
    if (tmpDoc == NULL) {
        return false;
    }

    // Validation the same as in the previous function, using the XML struct
    xmlSchema *schema = NULL;
    xmlSchemaParserCtxt *newCtxt = NULL;

    xmlLineNumbersDefault(1);

    newCtxt = xmlSchemaNewParserCtxt(gpxSchemaFile);

    schema = xmlSchemaParse(newCtxt);
    xmlSchemaFreeParserCtxt(newCtxt);

    xmlSchemaValidCtxt *ctxt = xmlSchemaNewValidCtxt(schema);
    int ret = xmlSchemaValidateDoc(ctxt, tmpDoc);
    xmlSchemaFreeValidCtxt(ctxt);
    xmlSchemaFree(schema);
    xmlSchemaCleanupTypes();


    xmlFreeDoc(tmpDoc);
    xmlCleanupParser();

    if (ret != 0) {
        return false;
    }

    // Check other constraints
    if (doc->namespace[0] == '\0') {
        return false;
    }

    if (doc->creator == NULL || doc->creator[0] == '\0') {
        return false;
    }

    if (doc->waypoints == NULL || doc->routes == NULL || doc->tracks == NULL) {
        return false;
    }

    if (checkWaypoints(doc->waypoints) != 0 || checkRoutes(doc->routes) != 0 || checkTracks(doc->tracks) != 0) {
        return false;
    }

    return true;

}

// Write a GPX doc struct to file
bool writeGPXdoc(GPXdoc *doc, char *fileName) {

    // Error checking
    if (doc == NULL || fileName == NULL) {
        return false;
    }

    if (strlen(fileName) == 0) {
        return false;
    } else if (fileName[0] == ' ') {
        return false;
    }

    xmlDoc *tmpDoc = gpxDocToXMLDoc(doc);
    if (tmpDoc == NULL) {
        return false;
    }

    // Try and save the file, return false if it failed
    if (xmlSaveFormatFileEnc(fileName, tmpDoc, "UTF-8", 1) == -1) {
        xmlFreeDoc(tmpDoc);
        xmlCleanupParser();
        return false;
    }

    // Freeing
    xmlFreeDoc(tmpDoc);
    xmlCleanupParser();

    return true;

}

// Get route length from a route
float getRouteLen (const Route *rt) {

    if (rt == NULL) {
        return 0.0;
    }

    return getTotalWaypointsLen(rt->waypoints);

}

// Get track length from a track
float getTrackLen (const Track *tr) {

    if (tr == NULL) {
        return 0.0;
    }

    return getTotalTrackSegLen(tr->segments);

}

// Rounding function to round a float to the nearest multiple of 10
float round10 (float len) {

    if (len < 0) {
        len = len * (-1);
    }

    return 10 * (int) round((len/10));

}

// Function to find the number of routes with a specific length ± delta value
int numRoutesWithLength(const GPXdoc* doc, float len, float delta) {

    int total = 0;

    // Error checking
    if (doc == NULL || len < 0 || delta < 0) {
        return total;
    }

    if (doc->routes == NULL) {
        return total;
    }

    void *elem;
    List *routeList = doc->routes;
    ListIterator routeIter = createIterator(routeList);

    // Loop through all routes in the doc
	while ((elem = nextElement(&routeIter)) != NULL) {

        // Cast to Route
        Route *tmpRte = (Route *)elem;

        float length = getRouteLen(tmpRte);

        // If the lengths are within delta of each other, increment count
        if (fabs(length - len) <= delta) {
            total++;
        }

	}
    
    return total;
    
}

// Same as the last function, except for tracks instead
int numTracksWithLength(const GPXdoc* doc, float len, float delta) {

    int total = 0;

    if (doc == NULL || len < 0 || delta < 0) {
        return total;
    }

    if (doc->tracks == NULL) {
        return total;
    }

    void *elem;
    List *trackList = doc->tracks;
    ListIterator trackIter = createIterator(trackList);

	while ((elem = nextElement(&trackIter)) != NULL) {

        Track *tmpTrk = (Track *)elem;

        float length = getTrackLen(tmpTrk);

        if (fabs(length - len) <= delta) {
            total++;
        }

	}
    
    return total;

}

// Check if a route is a loop, has to have more than 4 points, and the first and last point must be within delta of each other
bool isLoopRoute(const Route* route, float delta) {

    if (route == NULL || delta < 0) {
        return false;
    }

    if (route->waypoints == NULL) {
        return false;
    }

    if (getLength(route->waypoints) < 4) {
        return false;
    }

    // Get the first and last points in the route
    Waypoint *tmpWpt1 = getFromFront(route->waypoints);
    Waypoint *tmpWpt2 = getFromBack(route->waypoints);

    // If the distance is within delta, it is a loop
    if (haversine(tmpWpt1->latitude, tmpWpt1->longitude, tmpWpt2->latitude, tmpWpt2->longitude) < delta) {
        return true;
    }

    return false;

}

// Same as the last function, except for tracks
bool isLoopTrack(const Track *tr, float delta) {

    if (tr == NULL || delta < 0) {
        return false;
    }

    if (tr->segments == NULL) {
        return false;
    }

    int totalLength = 0;

    void *elem;
    List *trackSegList = tr->segments;
    ListIterator trackSegIter = createIterator(trackSegList);

    // Count the total number of track points (in all segments), to see if there is more than 4
    while((elem = nextElement(&trackSegIter)) != NULL) {

        TrackSegment *tmpTrkSeg = (TrackSegment *)elem;
        totalLength += getLength(tmpTrkSeg->waypoints);

    }

    if (totalLength < 4) {
        return false;
    }

    // Get first point from first segment
    TrackSegment *tmpSeg1 = getFromFront(tr->segments);
    Waypoint *tmpWpt1 = getFromFront(tmpSeg1->waypoints);

    // Get last point from last segment
    TrackSegment *tmpSeg2 = getFromBack(tr->segments);
    Waypoint *tmpWpt2 = getFromBack(tmpSeg2->waypoints);

    // If the distance is within delta of each other, it is a loop
    if (haversine(tmpWpt1->latitude, tmpWpt1->longitude, tmpWpt2->latitude, tmpWpt2->longitude) < delta) {
        return true;
    }

    return false;

}

// Get a list of all the routes between two points
List *getRoutesBetween(const GPXdoc* doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta) {

    if (doc == NULL || doc->routes == NULL || delta < 0) {
        return NULL;
    }

    // Initialize new list, with a dummy delete, so it will not delete the originals when freed
    List *tmpList = initializeList(&routeToString, &dummyDelete, &compareRoutes);

    void *elem;
    List *routeList = doc->routes;
    ListIterator routeIter = createIterator(routeList);

    int count = 0;
	while ((elem = nextElement(&routeIter)) != NULL) {

        Route *tmpRte = (Route *)elem;

        Waypoint *tmpWpt1 = getFromFront(tmpRte->waypoints);
        if (tmpWpt1 == NULL) {
            continue;
        }

        // If the distance between the source latitude and longitude, and the first point of the route is within delta, check the last points as well
        if (haversine(tmpWpt1->latitude, tmpWpt1->longitude, sourceLat, sourceLong) <= delta) {

            Waypoint *tmpWpt2 = getFromBack(tmpRte->waypoints);

            // If the last points are also within delta of each other, then add to the list
            if (haversine(tmpWpt2->latitude, tmpWpt2->longitude, destLat, destLong) <= delta) {
                insertBack(tmpList, tmpRte);
                count++;
            }

        }

	}

    // If the list is empty, free it and return NULL
    if (count == 0) {
        freeList(tmpList);
        return NULL;
    }

    return tmpList;

}

// Same function as previous, except for tracks
List *getTracksBetween(const GPXdoc* doc, float sourceLat, float sourceLong, float destLat, float destLong, float delta) {

    if (doc == NULL || doc->tracks == NULL || delta < 0) {
        return NULL;
    }

    List *tmpList = initializeList(&trackToString, &dummyDelete, &compareTracks);

    void *elem;
    List *trackList = doc->tracks;
    ListIterator trackIter = createIterator(trackList);

    int count = 0;
	while ((elem = nextElement(&trackIter)) != NULL) {


        Track *tmpTrk = (Track *)elem;

        TrackSegment *tmpSeg1 = getFromFront(tmpTrk->segments);
        if (tmpSeg1 == NULL) {
            continue;
        }

        Waypoint *tmpWpt1 = getFromFront(tmpSeg1->waypoints);
        if (tmpWpt1 == NULL) {
            continue;
        }

        if (haversine(tmpWpt1->latitude, tmpWpt1->longitude, sourceLat, sourceLong) <= delta) {

            TrackSegment *tmpSeg2 = getFromBack(tmpTrk->segments);
            Waypoint *tmpWpt2 = getFromBack(tmpSeg2->waypoints);

            if (haversine(tmpWpt2->latitude, tmpWpt2->longitude, destLat, destLong) <= delta) {
                insertBack(tmpList, tmpTrk);
                count++;
            }

        }

	}

    if (count == 0) {
        freeList(tmpList);
        return NULL;
    }
    
    return tmpList;

}

// Convert a track to a string in JSON format
char *trackToJSON(const Track *tr) {

    // Malloc enough space for the brackets and null terminator
    int totalLength = 3;
    char *retString = malloc(totalLength);

    // If the track is NULL, return an empty JSON 'object'
    if (tr == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    char *tmpName;
    // If the track has no name
    if (tr->name[0] == '\0') {
        // Realloc space for 4 extra characters
        retString = realloc(retString, totalLength + 4);
        tmpName = malloc(5);
        // name becomes "None"
        strcpy(tmpName, "None");
    } else {
        // Otherwise realloc enough space for the name, and copy the name into tmpName
        int nameLength = strlen(tr->name);
        totalLength += nameLength;
        retString = realloc(retString, totalLength);
        tmpName = malloc(nameLength + 1);
        strcpy(tmpName, tr->name);
    }

    // Track length should have maximum 100 chars

    if (isLoopTrack(tr, 10)) {
        totalLength += 4; // For true
    } else {
        totalLength += 5; // For false
    }
    totalLength += 124; // For the labels

    // Realloc the total length
    retString = realloc(retString, totalLength);

    // Copy the string in
    sprintf(retString, "{\"name\":\"%s\",\"len\":%.1f,\"loop\":%s}", tmpName, round10(getTrackLen(tr)), isLoopTrack(tr, 10) ? "true" : "false");

    // Free temp name variable
    free(tmpName);
    
    return retString;

}

// Same as the last function but for route, so different fields
char *routeToJSON(const Route *rt) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (rt == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    char *tmpName;
    if (rt->name[0] == '\0') {
        retString = realloc(retString, totalLength + 4);
        tmpName = malloc(5);
        strcpy(tmpName, "None");
    } else {
        int nameLength = strlen(rt->name);
        totalLength += nameLength;
        retString = realloc(retString, totalLength);
        tmpName = malloc(nameLength + 1);
        strcpy(tmpName, rt->name);
    }

    // numPoints number should have maximum 100 chars
    // same for routeLen

    if (isLoopRoute(rt, 10)) {
        totalLength += 4; // For true
    } else {
        totalLength += 5; // for false
    }
    totalLength += 237; // For the labels

    retString = realloc(retString, totalLength);
    sprintf(retString, "{\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", tmpName, getLength(rt->waypoints), round10(getRouteLen(rt)), isLoopRoute(rt, 10) ? "true" : "false");
    
    free(tmpName);

    return retString;
    
}

// Convert a route list to a JSON string
char *routeListToJSON(const List *list) {

    // Malloc enough space for the brackets
    int totalLength = 3;
    char *retString = malloc(totalLength);

    // If the list is empty, copy in the brackets and return
    if (list == NULL) {
        strcpy(retString, "[]");
        return retString;
    }

    void *elem;
    List *routeList = (List *)list;
    ListIterator routeIter = createIterator(routeList);

    // Start the list with the first bracket
    strcpy(retString, "[");

    int lastIndex = getLength(routeList) - 1;
    int i = 0;
	while ((elem = nextElement(&routeIter)) != NULL) {

        // Cast to Route
        Route *tmpRte = (Route *)elem;

        // Convert the route to JSON using previous function
        char *routeJSONString = routeToJSON(tmpRte);
        int lengthOfJSONString = strlen(routeJSONString);

        // If it is not the last route, then copy a comma after every string
        if (i != lastIndex) {
            totalLength += lengthOfJSONString + 1; // 1 for the comma
            retString = realloc(retString, totalLength);
            strcat(retString, routeJSONString);
            strcat(retString, ",");
        } else {
            totalLength += lengthOfJSONString;
            retString = realloc(retString, totalLength);
            strcat(retString, routeJSONString);
        }
        free(routeJSONString);

        i++;

	}

    // Close the list and return
    strcat(retString, "]");

    return retString;

}

// Same as previous function, except for lists of tracks
char *trackListToJSON(const List *list) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (list == NULL) {
        strcpy(retString, "[]");
        return retString;
    }

    void *elem;
    List *trackList = (List *)list;
    ListIterator trackIter = createIterator(trackList);

    strcpy(retString, "[");

    int lastIndex = getLength(trackList) - 1;
    int i = 0;
	while ((elem = nextElement(&trackIter)) != NULL) {

        Track *tmpTrk = (Track *)elem;

        char *trackJSONString = trackToJSON(tmpTrk);
        int lengthOfJSONString = strlen(trackJSONString);

        if (i != lastIndex) {
            totalLength += lengthOfJSONString + 1;
            retString = realloc(retString, totalLength);
            strcat(retString, trackJSONString);
            strcat(retString, ",");
        } else {
            totalLength += lengthOfJSONString;
            retString = realloc(retString, totalLength);
            strcat(retString, trackJSONString);
        }
        free(trackJSONString);

        i++;

	}

    strcat(retString, "]");

    return retString;

}

// Convert a GPX doc to JSON string
char *GPXtoJSON(const GPXdoc *gpx) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    // Error checking
    if (gpx == NULL || gpx->creator == NULL || gpx->creator[0] == '\0') {
        strcpy(retString, "{}");
        return retString;
    }

    // Version number should have maximum 100 chars

    totalLength += 464 + strlen(gpx->creator); // For the labels and 300 for the other 3 numbers
    retString = realloc(retString, totalLength);

    sprintf(retString, "{\"version\":%g,\"creator\":\"%s\",\"numWaypoints\":%d,\"numRoutes\":%d,\"numTracks\":%d}", gpx->version, gpx->creator, getNumWaypoints(gpx), getNumRoutes(gpx), getNumTracks(gpx));

    return retString;

}

// Add a point to a route
void addWaypoint(Route *rt, Waypoint *pt) {

    if (rt == NULL || rt->waypoints == NULL || pt == NULL) {
        return;
    }

    insertBack(rt->waypoints, pt);

}

// Add a route to a GPXdoc
void addRoute(GPXdoc *doc, Route *rt) {

    if (doc == NULL || doc->routes == NULL || rt == NULL) {
        return;
    }

    insertBack(doc->routes, rt);

}

// Convert a JSON string to GPXdoc
GPXdoc *JSONtoGPX(const char *gpxString) {

    if (gpxString == NULL) {
        return NULL;
    }

    char *tmpStr = malloc(strlen(gpxString) + 1);
    strcpy(tmpStr, gpxString);

    GPXdoc *newGPXDoc = malloc(sizeof(GPXdoc));
    if (newGPXDoc == NULL) {
        return NULL;
    }

    // The separators to get only the values
    char separators[6] = "{}:,\"";
    char *token = strtok(tmpStr, separators);
    int i = 0;
    char tokens[4][100];
    while (token != NULL) {
        strcpy(tokens[i], token);
        token = strtok(NULL, separators);
        i++;
    }
    free(tmpStr);

    for (int j = 0; j < 4; j++) {
        if (strcmp(tokens[j], "version") == 0) {
            newGPXDoc->version = atof(tokens[j + 1]);
        } else if (strcmp(tokens[j], "creator") == 0) {
            newGPXDoc->creator = malloc(strlen(tokens[j + 1]) + 1);
            strcpy(newGPXDoc->creator, tokens[j + 1]);
        }
    }

    // Namespace will always be the same
    strcpy(newGPXDoc->namespace, "http://www.topografix.com/GPX/1/1");

    // Initialize lists
    newGPXDoc->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    newGPXDoc->routes = initializeList(&routeToString, &deleteRoute, &compareRoutes);
    newGPXDoc->tracks = initializeList(&trackToString, &deleteTrack, &compareTracks);

    return newGPXDoc;

}

// Similar to last function, but for waypoints
Waypoint *JSONtoWaypoint(const char* gpxString) {

    if (gpxString == NULL) {
        return NULL;
    }

    char *tmpStr = malloc(strlen(gpxString) + 1);
    strcpy(tmpStr, gpxString);

    Waypoint *newWaypoint = malloc(sizeof(Waypoint));
    if (newWaypoint == NULL) {
        return NULL;
    }

    char separators[6] = "{}:,\"";
    char *token = strtok(tmpStr, separators);
    int i = 0;
    char tokens[4][100];
    while (token != NULL) {
        strcpy(tokens[i], token);
        token = strtok(NULL, separators);
        i++;
    }
    free(tmpStr);

    for (int j = 0; j < 4; j++) {
        if (strcmp(tokens[j], "lat") == 0) {
            newWaypoint->latitude = atof(tokens[j + 1]);
        } else if (strcmp(tokens[j], "lon") == 0) {
            newWaypoint->longitude = atof(tokens[j + 1]);
        }
    }

    newWaypoint->name = malloc(1);
    newWaypoint->name[0] = '\0';

    newWaypoint->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    return newWaypoint;

}

// Similar to last functions but for routes
Route *JSONtoRoute(const char* gpxString) {

    if (gpxString == NULL) {
        return NULL;
    }

    char *tmpStr = malloc(strlen(gpxString) + 1);
    strcpy(tmpStr, gpxString);

    Route *newRoute = malloc(sizeof(Route));
    if (newRoute == NULL) {
        return NULL;
    }

    char separators[6] = "{}:,\"";
    char *token = strtok(tmpStr, separators);
    int i = 0;
    char tokens[2][100];
    while (token != NULL) {
        strcpy(tokens[i], token);
        token = strtok(NULL, separators);
        i++;
    }
    free(tmpStr);

    for (int j = 0; j < 2; j++) {
        if (strcmp(tokens[j], "name") == 0) {
            printf("%s\n",tokens[j + 1]);
            newRoute->name = malloc(strlen(tokens[j + 1]) + 1);
            strcpy(newRoute->name, tokens[j + 1]);
        }
    }

    newRoute->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    newRoute->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

    return newRoute;

}

/**                               WRAPPER FUNCTIONS FOR BACKEND                               **/

// Get the GPXdata of a file after validating
char *getGPXDataIfValid (char *gpxFile, char *schemaFile) {

    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, schemaFile);

    char *retString = GPXtoJSON(tmpGPXDoc);

    deleteGPXdoc(tmpGPXDoc);

    return retString;

}

// New version of the trackToJSON, which has an extra field, which is num points
char *newTrackToJSON (const Track *tr) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (tr == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    char *tmpName;
    if (tr->name[0] == '\0') {
        retString = realloc(retString, totalLength + 4);
        tmpName = malloc(5);
        strcpy(tmpName, "None");
    } else {
        int nameLength = strlen(tr->name);
        totalLength += nameLength;
        retString = realloc(retString, totalLength);
        tmpName = malloc(nameLength + 1);
        strcpy(tmpName, tr->name);
    }

    // Tracklen should have maximum 100 chars

    if (isLoopTrack(tr, 10)) {
        totalLength += 4; // For true
    } else {
        totalLength += 5; // For false
    }
    totalLength += 160; // For the labels
    retString = realloc(retString, totalLength);

    int numPoints = 0;

    void *elem;
    List *trackSegList = tr->segments;
    ListIterator trackSegIter = createIterator(trackSegList);

    while((elem = nextElement(&trackSegIter)) != NULL) {

        TrackSegment *tmpTrkSeg = (TrackSegment *)elem;
        numPoints += getLength(tmpTrkSeg->waypoints);

    }

    sprintf(retString, "{\"name\":\"%s\",\"numPoints\":%d,\"len\":%.1f,\"loop\":%s}", tmpName, numPoints ,round10(getTrackLen(tr)), isLoopTrack(tr, 10) ? "true" : "false");
    free(tmpName);
    
    return retString;

}

// New trackListToJSON as well, to incorporate previous change
char *newTrackListToJSON (const List *list) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (list == NULL) {
        strcpy(retString, "[]");
        return retString;
    }

    void *elem;
    List *trackList = (List *)list;
    ListIterator trackIter = createIterator(trackList);

    strcpy(retString, "[");

    int lastIndex = getLength(trackList) - 1;
    int i = 0;
	while ((elem = nextElement(&trackIter)) != NULL) {

        Track *tmpTrk = (Track *)elem;

        char *trackJSONString = newTrackToJSON(tmpTrk);
        int lengthOfJSONString = strlen(trackJSONString);
        if (i != lastIndex) {
            totalLength += lengthOfJSONString + 1;
            retString = realloc(retString, totalLength);
            strcat(retString, trackJSONString);
            strcat(retString, ",");
        } else {
            totalLength += lengthOfJSONString;
            retString = realloc(retString, totalLength);
            strcat(retString, trackJSONString);
        }
        free(trackJSONString);

        i++;

	}

    strcat(retString, "]");

    return retString;

}

// Get the routes and tracks information from a file, in that order
char *getRoutesAndTracksFromFile (char *gpxFile, char *schemaFile) {

    // Create a valid GPXdoc struct
    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, schemaFile);

    // Get the route list and track list separately
    char *routeListString = routeListToJSON(tmpGPXDoc->routes);
    char *trackListString = newTrackListToJSON(tmpGPXDoc->tracks);

    // Malloc enough space for both strings, labels and null terminator
    char *retString = malloc(strlen(routeListString) + strlen(trackListString) + 22);

    // Copy into return string
    sprintf(retString, "{\"routes\":%s,\"tracks\":%s}", routeListString, trackListString);

    // Free other strings and delete the temporary GPXdoc
    free(routeListString);
    free(trackListString);
    deleteGPXdoc(tmpGPXDoc);

    return retString;

}

// Similar function to before, this time for converting otherData to JSON format
char *gpxDataToJSON (GPXData *data) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (data == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    int nameLength = strlen(data->name);
    totalLength += nameLength;
    retString = realloc(retString, totalLength);
    char *tmpName = malloc(nameLength + 1);
    strcpy(tmpName, data->name);

    int dataLength = strlen(data->value);

    for (int i = 0; i < dataLength; i++) {
        if (data->value[i] == '\n') {
            data->value[i] = ' ';
        }
    }

    totalLength += dataLength;
    retString = realloc(retString, totalLength);
    char *value = malloc(dataLength + 1);
    strcpy(value, data->value);

    totalLength += 20;
    retString = realloc(retString, totalLength);

    sprintf(retString, "{\"name\":\"%s\",\"value\":\"%s\"}", tmpName, value);
    free(tmpName);
    free(value);

    return retString;

}

// Simialr function again, for a list of otherData
char *gpxDataListToJSON (List *otherDataList) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (otherDataList == NULL) {
        strcpy(retString, "[]");
        return retString;
    }

    void *elem;
    ListIterator dataIter = createIterator(otherDataList);

    strcpy(retString, "[");

    int lastIndex = getLength(otherDataList) - 1;
    int i = 0;
	while ((elem = nextElement(&dataIter)) != NULL) {

        GPXData *tmpData = (GPXData *)elem;

        char *dataString = gpxDataToJSON(tmpData);
        int lengthOfJSONString = strlen(dataString);
        if (i != lastIndex) {
            totalLength += lengthOfJSONString + 1;
            retString = realloc(retString, totalLength);
            strcat(retString, dataString);
            strcat(retString, ",");
        } else {
            totalLength += lengthOfJSONString;
            retString = realloc(retString, totalLength);
            strcat(retString, dataString);
        }
        free(dataString);

        i++;

	}

    strcat(retString, "]");

    return retString;

}

// Get otherData based on route/track index in the original file
char *getOtherData (char *gpxFile, char *schemaFile, int type, int index) {

    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, schemaFile);

    // Routes
    if (type == 1) {

        void *elem;
        List *routeList = tmpGPXDoc->routes;
        ListIterator routeIter = createIterator(routeList);

        char *retString = NULL;

        // Loop through until the index is hit and return the otherData list in JSON format
        int i = 1;
        while((elem = nextElement(&routeIter)) != NULL) {

            Route *tmpRoute = (Route *)elem;
            if (i == index) {
                retString = gpxDataListToJSON(tmpRoute->otherData);
                deleteGPXdoc(tmpGPXDoc);
                return retString;
            }

            i++;
        }

        retString = malloc(3);
        strcpy(retString, "[]");

        deleteGPXdoc(tmpGPXDoc);

        return retString;

    } else {

        // Tracks

        void *elem;
        List *trackList = tmpGPXDoc->tracks;
        ListIterator trackIter = createIterator(trackList);

        char *retString = NULL;

        // Loop through until the index is hit and return the otherData list in JSON format
        int i = 1;
        while((elem = nextElement(&trackIter)) != NULL) {

            Track *tmpTrack = (Track *)elem;
            if (i == index) {
                retString = gpxDataListToJSON(tmpTrack->otherData);
                deleteGPXdoc(tmpGPXDoc);
                return retString;
            }

            i++;
        }

        retString = malloc(3);
        strcpy(retString, "[]");

        deleteGPXdoc(tmpGPXDoc);

        return retString;

    }

}

// Rename a route/track based on index in the original file
int renameRoute (char *gpxFile, char *schemaFile, int type, int index, char *newName) {

    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, schemaFile);

    // Routes
    if (type == 1) {

        void *elem;
        List *routeList = tmpGPXDoc->routes;
        ListIterator routeIter = createIterator(routeList);

        int i = 1;
        while((elem = nextElement(&routeIter)) != NULL) {

            Route *tmpRoute = (Route *)elem;

            if (i == index) {
                int newNameLength = strlen(newName);
                // If more space is needed for the name, realloc enough space
                if (newNameLength > strlen(tmpRoute->name)) {
                    tmpRoute->name = realloc(tmpRoute->name, newNameLength + 1);
                }
                // Copy in the new name to the struct
                strcpy(tmpRoute->name, newName);

                // Write the struct back to same file to update changes
                if (writeGPXdoc(tmpGPXDoc, gpxFile)) {
                    deleteGPXdoc(tmpGPXDoc);
                    return 1;
                } else {
                    deleteGPXdoc(tmpGPXDoc);
                    return 0;
                }
            }

            i++;
        }

        deleteGPXdoc(tmpGPXDoc);

        // Return 0 on fail
        return 0;

    } else {

        // Tracks

        void *elem;
        List *trackList = tmpGPXDoc->tracks;
        ListIterator trackIter = createIterator(trackList);

        int i = 1;
        while((elem = nextElement(&trackIter)) != NULL) {

            Track *tmpTrack = (Track *)elem;
            if (i == index) {
                int newNameLength = strlen(newName);
                if (newNameLength > strlen(tmpTrack->name)) {
                    tmpTrack->name = realloc(tmpTrack->name, newNameLength + 1);
                }
                strcpy(tmpTrack->name, newName);
                if (writeGPXdoc(tmpGPXDoc, gpxFile)) {
                    deleteGPXdoc(tmpGPXDoc);
                    return 1;
                } else {
                    deleteGPXdoc(tmpGPXDoc);
                    return 0;
                }
            }

            i++;
        }

        deleteGPXdoc(tmpGPXDoc);

        // Return 0 on fail
        return 0;

    }

}

// Create an empty GPX file
int createEmptyGPX (char *outputFilename, char *creator) {

    GPXdoc *newDoc = malloc(sizeof(GPXdoc));

    // Default version and namespace
    newDoc->version = 1.1;
    strcpy(newDoc->namespace, "http://www.topografix.com/GPX/1/1");

    // Copy in creator name
    newDoc->creator = malloc(strlen(creator) + 1);
    strcpy(newDoc->creator, creator);

    // Initialize lists
    newDoc->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
    newDoc->routes = initializeList(&routeToString, &deleteRoute, &compareRoutes);
    newDoc->tracks = initializeList(&trackToString, &deleteTrack, &compareTracks);

    if (!validateGPXDoc(newDoc, "gpx.xsd")) {
        deleteGPXdoc(newDoc);
        return 0;
    }

    // Write to file
    if (writeGPXdoc(newDoc, outputFilename)) {
        return 1;
    }

    return 0;

}

// Wrapper function to add a new route to a file
int addRouteToFile (char *gpxFile, char *routeNameJSON) {

    // Create a new route from the JSON string
    Route *newRoute = JSONtoRoute(routeNameJSON);

    // Create a temporary GPXdoc struct
    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (tmpGPXDoc == NULL) {
        return 0;
    }

    // Add the route to the GPXdoc struct
    addRoute(tmpGPXDoc, newRoute);

    // Attempt to write to file
    if (writeGPXdoc(tmpGPXDoc, gpxFile)) {
        deleteGPXdoc(tmpGPXDoc);
        return 1;
    } else {
        deleteGPXdoc(tmpGPXDoc);
        return 0;
    }

}

// Add a waypoint to a route in a file
int addWaypointToRouteInFile (char *gpxFile, char *waypointJSON) {

    // Create a valid GPXdoc struct
    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (tmpGPXDoc == NULL) {
        return 0;
    }

    // It will always be the latest route that was added
    Route *tmpRoute = getFromBack(tmpGPXDoc->routes);

    // Convert the JSON string to a Waypoint
    Waypoint *waypointToAdd = JSONtoWaypoint(waypointJSON);

    // Add the waypoint to the route
    addWaypoint(tmpRoute, waypointToAdd);

    // Attempt to write to file
    if (writeGPXdoc(tmpGPXDoc, gpxFile)) {
        deleteGPXdoc(tmpGPXDoc);
        return 1;
    } else {
        deleteGPXdoc(tmpGPXDoc);
        return 0;
    }

}

// Alternate version of getRoutesBetween, with return format of JSON string instead of List
char *getRoutesBetweenJSON (char *gpxFile, float lat1, float lon1, float lat2, float lon2, float delta) {

    char *retString = NULL;

    // Try and create valid GPXdoc
    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (tmpGPXDoc == NULL) {
        retString = malloc(3);
        strcpy(retString, "{}");
        return retString;
    }

    // Get rourtes between points
    List *routeList = getRoutesBetween(tmpGPXDoc, lat1, lon1, lat2, lon2, delta);

    // Convert list to JSON
    retString = routeListToJSON(routeList);

    // Delete temporary GPXdoc
    deleteGPXdoc(tmpGPXDoc);

    return retString;

}

// Same as last function but for tracks
char *getTracksBetweenJSON (char *gpxFile, float lat1, float lon1, float lat2, float lon2, float delta) {

    char *retString = NULL;

    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (tmpGPXDoc == NULL) {
        retString = malloc(3);
        strcpy(retString, "{}");
        return retString;
    }

    List *trackList = getTracksBetween(tmpGPXDoc, lat1, lon1, lat2, lon2, delta);

    retString = newTrackListToJSON(trackList);

    deleteGPXdoc(tmpGPXDoc);

    return retString;

}

// Get paths with specific length
char *getPathsWithLength (char *gpxFile, float length) {

    char *retString = malloc(3);

    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (tmpGPXDoc == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    // Get routes/tracks with the specific length, default delta value of 10
    int routesWithLen = numRoutesWithLength(tmpGPXDoc, length, 10);
    int tracksWithLen = numTracksWithLength(tmpGPXDoc, length, 10);

    // Copy the formatted JSON return string
    retString = realloc(retString, 80);
    sprintf(retString, "{\"rt\":%d,\"tr\":%d}", routesWithLen, tracksWithLen);
    
    return retString;

}

// Similar to previous toJSON functions, except for waypoint 
char *waypointToJSON (Waypoint *wpt) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    if (wpt == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    int nameLength = strlen(wpt->name);
    totalLength += nameLength;
    retString = realloc(retString, totalLength);
    char *tmpName = malloc(nameLength + 1);
    strcpy(tmpName, wpt->name);

    totalLength += 80;
    retString = realloc(retString, totalLength);

    sprintf(retString, "{\"name\":\"%s\",\"latitude\":%f,\"longitude\":%f}", tmpName, wpt->latitude, wpt->longitude);
    free(tmpName);

    return retString;

}

// Simialr to previous ListToJSON functions, except for waypoint lists
char *waypointListToJSON (char *gpxFile, int index) {

    int totalLength = 3;
    char *retString = malloc(totalLength);

    GPXdoc *doc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (doc == NULL) {
        strcpy(retString, "{}");
        return retString;
    }

    Route *tmpRte;
    ListIterator routeIter = createIterator(doc->routes);
    int j = 0;
    while ((tmpRte = nextElement(&routeIter)) != NULL) {
        if (j == index) {
            break;
        }
        j++;
    }

    List *list = tmpRte->waypoints;

    if (list == NULL) {
        strcpy(retString, "[]");
        deleteGPXdoc(doc);
        return retString;
    }

    void *elem;
    ListIterator dataIter = createIterator(list);

    strcpy(retString, "[");

    int lastIndex = getLength(list) - 1;
    int i = 0;
	while ((elem = nextElement(&dataIter)) != NULL) {

        Waypoint *tmpWpt = (Waypoint *)elem;

        char *dataString = waypointToJSON(tmpWpt);
        int lengthOfJSONString = strlen(dataString);
        if (i != lastIndex) {
            totalLength += lengthOfJSONString + 1; // 1 for the comma
            retString = realloc(retString, totalLength);
            strcat(retString, dataString);
            strcat(retString, ",");
        } else {
            totalLength += lengthOfJSONString;
            retString = realloc(retString, totalLength);
            strcat(retString, dataString);
        }
        free(dataString);

        i++;

	}

    strcat(retString, "]");

    deleteGPXdoc(doc);

    return retString;

}

// Return the last route of a particular file as a JSON string
char *lastRouteToJSON (char *gpxFile) {

    GPXdoc *tmpGPXDoc = createValidGPXdoc(gpxFile, "gpx.xsd");
    if (tmpGPXDoc == NULL) {
        return 0;
    }

    Route *tmpRoute = getFromBack(tmpGPXDoc->routes);

    char *retString = routeToJSON(tmpRoute);

    deleteGPXdoc(tmpGPXDoc);

    return retString;

}
