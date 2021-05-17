#include "GPXHelpers.h" // Included necessary header

// Function to insert a waypoint or similar into a given list
void insertWaypoints(xmlNode *cur_node, Waypoint *tmpWpt, List *listToInsertInto) {

    xmlNode *tmpIter; // Declare an iterator variable

    // Error checking 
    if (cur_node == NULL || tmpWpt == NULL || listToInsertInto == NULL) {
        deleteWaypoint(tmpWpt); // To prevent memory leaks
        return;
    }

    // Iterate through the children until NULL is hit
    for (tmpIter = cur_node->children; tmpIter != NULL; tmpIter = tmpIter->next) {

        // If node with name "name" is found
        if (strcmp((const char *)tmpIter->name, "name") == 0 ) {

            // Variable to store the content
            char *content = (char *)xmlNodeGetContent(tmpIter);
            
            // Malloc enough space for waypoint->name and then copy the name
            int length = strlen(content) + 1;
            tmpWpt->name = malloc(length);
            strcpy(tmpWpt->name, content);

            // Free to avoid leaks
            xmlFree(content);

        } else { // If any other node (which means otherData)

            // Variable to store the content
            char *content = (char *)xmlNodeGetContent(tmpIter);

            // If neither name nor value is empty
            if (tmpIter->name[0] != '\0' && !isspace(tmpIter->name[0]) && tmpIter->children && content[0] != '\0') {

                // Malloc enough space for the GPXData element
                GPXData *tmpData = malloc(sizeof(GPXData) + strlen(content) + 1);

                // Copying name and value
                strcpy(tmpData->name, (const char *)tmpIter->name);
                strcpy(tmpData->value, content);

                // Inserting into the otherData list of the waypoint
                insertBack(tmpWpt->otherData, tmpData);

            }

            // Free to avoid leaks
            xmlFree(content);

        }
    }

    xmlAttr *attr; // Declare an iterator variable for attributes
    int lat_count = 0, lon_count = 0; // Initialize variables to check if lat and lon exist (in order to be valid)

    // Iterate through every attribute of the current node until NULL is hit
    for (attr = cur_node->properties; attr != NULL; attr = attr->next)
    {
        // Assign variables for the value (and its content) and the name of the attribute
        xmlNode *value = attr->children;
        char *attrName = (char *)attr->name;
        char *cont = (char *)(value->content);

        // If the attribute name is "lat"
        if (strcmp((const char *)attrName, "lat") == 0) {

            // Copy into the waypoint struct's lat variable
            tmpWpt->latitude = atof(cont);

            // Increment count
            lat_count++;

        } else if (strcmp((const char *)attrName, "lon") == 0) { // If the attribute name is "lon"

            // Copy into the waypoint struct's lon variable
            tmpWpt->longitude = atof(cont);

            // Increment count
            lon_count++;

        }
    }

    // If lat or lon are not attributes, waypoint is invalid, so delete the waypoint and return 
    if (lat_count == 0 || lon_count == 0) {
        deleteWaypoint(tmpWpt);
        return;
    }

    // If name was not copied, because it had no name, assign an empty string
    if (tmpWpt->name == NULL) {
        tmpWpt->name = malloc(1);
        tmpWpt->name[0] = '\0';
    }

    // Insert waypoint into the given list
    insertBack(listToInsertInto, tmpWpt);

}

// Function to read through the XML tree and parse information into GPXdoc
void recursiveReader(xmlNode *a_node, GPXdoc *docToEdit) {

    // Initialize iterator
    xmlNode *cur_node = NULL;

    // Loop through all nodes until NULL is hit
    for (cur_node = a_node; cur_node != NULL; cur_node = cur_node->next) {

        // If the current node is readable (XML_ELEMENT_NODE)
        if (cur_node->type == XML_ELEMENT_NODE) {

            // If the tag is wpt
            if (strcmp((const char *)cur_node->name, "wpt") == 0) {

                // Malloc space for a new Waypoint
                Waypoint *tmpWpt = malloc(sizeof(Waypoint));

                // Name starts off as NULL for checking inside the function
                tmpWpt->name = NULL;

                // Initialize list in case of any otherData, cannot be NULL
                tmpWpt->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

                // Call insertWaypoints function to read waypoint from cur_node and add to the list if it is valid
                insertWaypoints(cur_node, tmpWpt, docToEdit->waypoints);

            } else if (strcmp((const char *)cur_node->name, "rte") == 0) { // If the tag is rte

                // Malloc space for a new Route
                Route *tmpRte = malloc(sizeof(Route));

                // Name starts off as NULL for checking inside the function
                tmpRte->name = NULL;

                // Initialize lists in case of any waypoints/otherData, cannot be NULL
                tmpRte->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);
                tmpRte->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

                // Declare iterator
                xmlNode *tmpIter;

                // Loop through the cur_node's children until NULL is hit
                for (tmpIter = cur_node->children; tmpIter != NULL; tmpIter = tmpIter->next) {

                    // If the route's name tag is found
                    if (strcmp((const char *)tmpIter->name, "name") == 0 ) {

                        // Variable to store the content
                        char *content = (char *)xmlNodeGetContent(tmpIter);

                        // Malloc enough space for the name and copy it
                        int length = strlen(content) + 1;
                        tmpRte->name = malloc(length);
                        strcpy(tmpRte->name, content);

                        // Free to avoid leaks
                        xmlFree(content);

                    } else if (strcmp((const char *)tmpIter->name, "rtept") == 0 ) { // If the rtept tag is found
                        
                        // Same process as adding a Waypoint, just with a different list
                        Waypoint *newWpt = malloc(sizeof(Waypoint));
                        newWpt->name = NULL;
                        newWpt->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

                        // Call insertWaypoints function to read waypoint from tmpIter and add to the list if it is valid
                        insertWaypoints(tmpIter, newWpt, tmpRte->waypoints);

                    } else if (tmpIter->name[0] != '\0' && !isspace(tmpIter->name[0])
                        && tmpIter->children && tmpIter->children->content[0] != '\0') { // anything else is otherData

                        // Same process as previous otherData
                        char *content = (char *)xmlNodeGetContent(tmpIter);

                        GPXData *tmpData = malloc(sizeof(GPXData) + strlen(content) + 1);

                        strcpy(tmpData->name, (const char *)tmpIter->name);
                        strcpy(tmpData->value, content);

                        insertBack(tmpRte->otherData, tmpData);

                        xmlFree(content);
                    }
                }

                // Same check at the end of the insertWaypoints function
                // if the name is NULL, then assign it an empty string
                if (tmpRte->name == NULL) {
                    tmpRte->name = malloc(1);
                    tmpRte->name[0] = '\0';
                }

                // Insert new Route into GPXdoc's routes list
                insertBack(docToEdit->routes, tmpRte);

            } else if (strcmp((const char *)cur_node->name, "trk") == 0) { // If the tag is trk

                // Malloc space for a new Track
                Track *tmpTrk = malloc(sizeof(Track));

                // Name starts off as NULL for checking inside the function
                tmpTrk->name = NULL;

                // Initialize lists in case of any segments/otherData, cannot be NULL
                tmpTrk->segments = initializeList(&trackSegmentToString, &deleteTrackSegment, &compareTrackSegments);
                tmpTrk->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

                // Declare iterator
                xmlNode *tmpIter;

                // Loop through the cur_node's children until NULL is hit
                for (tmpIter = cur_node->children; tmpIter != NULL; tmpIter = tmpIter->next) {

                    // If the track's name tag is found
                    if (strcmp((const char *)tmpIter->name, "name") == 0 ) {

                        // Variable to store the content
                        char *content = (char *)xmlNodeGetContent(tmpIter);

                        // Malloc enough space for the name and copy it
                        int length = strlen(content) + 1;
                        tmpTrk->name = malloc(length);
                        strcpy(tmpTrk->name, content);

                        // Free to avoid leaks
                        xmlFree(content);

                    } else if (strcmp((const char *)tmpIter->name, "trkseg") == 0 ) { // If the trkseg tag is found

                        // Malloc space for a new TrackSegment
                        TrackSegment *tmpTrkSeg = malloc(sizeof(TrackSegment));

                        // Initialize the waypoints list, cannot be NULL
                        tmpTrkSeg->waypoints = initializeList(&waypointToString, &deleteWaypoint, &compareWaypoints);

                        // Declare Iterator
                        xmlNode *newIter;

                        // Loop through tmpIter's children until NULL is hit
                        for (newIter = tmpIter->children; newIter != NULL; newIter = newIter->next) {

                            // If a trkpt is found
                            if (strcmp((const char *)newIter->name, "trkpt") == 0 ) {

                                // Malloc space for a new waypoint and initialize other variables
                                Waypoint *newWpt = malloc(sizeof(Waypoint));
                                newWpt->name = NULL;
                                newWpt->otherData = initializeList(&gpxDataToString, &deleteGpxData, &compareGpxData);

                                // Call insertWaypoints function to read waypoint from newIter and add to the list if it is valid
                                insertWaypoints(newIter, newWpt, tmpTrkSeg->waypoints);

                            }

                        }

                        // Insert new TrackSegment into segments list
                        insertBack(tmpTrk->segments, tmpTrkSeg);

                    } else if (tmpIter->name[0] != '\0' && !isspace(tmpIter->name[0])
                        && tmpIter->children && tmpIter->children->content[0] != '\0') { // Anything else is otherData
                        
                        // Same process as previous otherData
                        char *content = (char *)xmlNodeGetContent(tmpIter);
                        GPXData *tmpData = malloc(sizeof(GPXData) + strlen(content) + 1);
                        strcpy(tmpData->name, (const char *)tmpIter->name);
                        strcpy(tmpData->value, content);
                        insertBack(tmpTrk->otherData, tmpData);
                        xmlFree(content);

                    }

                }
                
                // Insert new Track into GPXdoc's tracks list
                insertBack(docToEdit->tracks, tmpTrk);

            }

        }

        // Recursive function call, because we are reading an XML tree
        recursiveReader(cur_node->children, docToEdit);

    }

}

// Add GPXdata to XML tree
int addGPXDataChildren(List *otherData, xmlNode *parentNode) {

    void *data;
    List *otherDataList = otherData;
    ListIterator otherDataIter = createIterator(otherDataList);

    while ((data = nextElement(&otherDataIter)) != NULL) {

        GPXData *tmpData = (GPXData *)data;

        if (tmpData->name[0] == '\0' || tmpData->value[0] == '\0') {
            return -1;
        }
        xmlNewChild(parentNode, NULL, BAD_CAST tmpData->name, BAD_CAST tmpData->value);

    }

    return 0;

}

// Add waypoints to XML tree
int addWaypointChildren(List *waypoints, xmlNode *parentNode, char *type) {

    // Iterate through the waypoints list
    void *elem;
    List *waypointList = waypoints;
    ListIterator waypointIter = createIterator(waypointList);

	while ((elem = nextElement(&waypointIter)) != NULL) {

        char buffer[256];

        // Cast to Waypoint
        Waypoint *tmpWpt = (Waypoint *)elem;

        xmlNode *waypointNode = xmlNewChild(parentNode, NULL, BAD_CAST type, NULL);

        // Error checking
        if ((tmpWpt->latitude > 90 || tmpWpt->latitude < -90) || (tmpWpt->longitude > 180 || tmpWpt->longitude < -180)) {
            return -1;
        }

        // Add latitude and longitude as properties to the new node
        sprintf(buffer, "%f", tmpWpt->latitude);
        xmlNewProp(waypointNode, BAD_CAST "lat", BAD_CAST buffer);
        sprintf(buffer, "%f", tmpWpt->longitude);
        xmlNewProp(waypointNode, BAD_CAST "lon", BAD_CAST buffer);

        // Invalid waypoint
        if (tmpWpt->name == NULL) {
            return -1;
        }
        // If no name, value is NULL
        if (tmpWpt->name[0] != '\0') {
            xmlNewChild(waypointNode, NULL, BAD_CAST "name", BAD_CAST tmpWpt->name);
        }

        // Try and add other data
        int childAdded = addGPXDataChildren(tmpWpt->otherData, waypointNode);

        if (childAdded != 0) {
            return -1;
        }

	}
    
    return 0;

}

// Add routes to XML tree
int addRouteChildren(List *routes, xmlNode *parentNode) {

    void *elem;
    List *routeList = routes;
    ListIterator routeIter = createIterator(routeList);

	while ((elem = nextElement(&routeIter)) != NULL) {

        // Cast to Waypoint
        Route *tmpRte = (Route *)elem;

        xmlNode *routeNode = xmlNewChild(parentNode, NULL, BAD_CAST "rte", NULL);

        if (tmpRte->name == NULL) {
            return -1;
        }
        if (tmpRte->name[0] != '\0') {
        xmlNewChild(routeNode, NULL, BAD_CAST "name", BAD_CAST tmpRte->name);
        }

        // Try and add other data
        int childAdded = addGPXDataChildren(tmpRte->otherData, routeNode);
        if (childAdded != 0) {
            return -1;
        }

        // Try and add the route points
        int rteptsAdded = addWaypointChildren(tmpRte->waypoints, routeNode, "rtept");
        if (rteptsAdded != 0) {
            return -1;
        }

	}
    
    return 0;

}

// Add track seg to XML tree
int addTrackSegChildren(List *trackSegs, xmlNode *parentNode) {

    void *elem;
    List *trackSegList = trackSegs;
    ListIterator trackSegIter = createIterator(trackSegList);

    while((elem = nextElement(&trackSegIter)) != NULL) {

        TrackSegment *tmpTrkSeg = (TrackSegment *)elem;

        xmlNode *trackSegmentNode = xmlNewChild(parentNode, NULL, BAD_CAST "trkseg", NULL);

        // Try and add wayppoints list to tree
        int tracksAdded = addWaypointChildren(tmpTrkSeg->waypoints, trackSegmentNode, "trkpt");
        if (tracksAdded != 0) {
            return -1;
        }

    }

    return 0;

}

// Add track to XML tree
int addTrackChildren(List *tracks, xmlNode *parentNode) {

    void *elem;
    List *trackList = tracks;
    ListIterator trackIter = createIterator(trackList);

	while ((elem = nextElement(&trackIter)) != NULL) {

        // Cast to Waypoint
        Track *tmpTrk = (Track *)elem;

        xmlNode *trackNode = xmlNewChild(parentNode, NULL, BAD_CAST "trk", NULL);

        if (tmpTrk->name == NULL) {
            return -1;
        }
        if (tmpTrk->name[0] != '\0') {
            xmlNewChild(trackNode, NULL, BAD_CAST "name", BAD_CAST tmpTrk->name);
        }

        // Try and add other data 
        int childAdded = addGPXDataChildren(tmpTrk->otherData, trackNode);
        if (childAdded != 0) {
            return -1;
        }

        // Try and add track segs
        int trksegsAdded = addTrackSegChildren(tmpTrk->segments, trackNode);
        if (trksegsAdded != 0) {
            return -1;
        }

	}
    
    return 0;

}

// Convert a GPXdoc struct to XML doc
xmlDoc *gpxDocToXMLDoc(GPXdoc *docToConvert) {

    if (docToConvert == NULL) {
        return NULL;
    }

    xmlDoc *doc = NULL;
    char buffer[256];

    LIBXML_TEST_VERSION;

    // Set root element of the XML tree as gpx
    doc = xmlNewDoc(BAD_CAST "1.0");
    xmlNode *rootNode = xmlNewNode(NULL, BAD_CAST "gpx");
    xmlDocSetRootElement(doc, rootNode);

    // Set version property on the gpx root element
    sprintf(buffer, "%g", docToConvert->version);
    xmlNewProp(rootNode, BAD_CAST "version", BAD_CAST buffer);
    if (docToConvert->creator == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }
    // Set creator property on the gpx root element
    xmlNewProp(rootNode, BAD_CAST "creator", BAD_CAST docToConvert->creator);
    if (docToConvert->namespace == NULL) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }
    // Set namespace
    xmlNs *namespace = xmlNewNs(rootNode, BAD_CAST docToConvert->namespace, NULL);
    xmlSetNs(rootNode, namespace);

    // Add all the waypoints to gpx root element as children
    int waypointsAdded = addWaypointChildren(docToConvert->waypoints, rootNode, "wpt");
    if (waypointsAdded != 0) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    // Add all the routes to gpx root element as children
    int routesAdded = addRouteChildren(docToConvert->routes, rootNode);
    if (routesAdded != 0) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    // Add all the tracks to gpx root element as children
    int tracksAdded = addTrackChildren(docToConvert->tracks, rootNode);
    if (tracksAdded != 0) {
        xmlFreeDoc(doc);
        xmlCleanupParser();
        return NULL;
    }

    // Cleanup any variables used by XML functions
    xmlCleanupParser();

    return doc;

}

// Check if list of GPX Data is valid
int checkGPXData(List *otherData) {

    void *data;
    List *otherDataList = otherData;
    ListIterator otherDataIter = createIterator(otherDataList);

    while ((data = nextElement(&otherDataIter)) != NULL) {

        GPXData *tmpData = (GPXData *)data;

        // name and value must not be NULL and have to be initialized
        if (tmpData->name[0] == '\0' || tmpData->value == NULL || tmpData->value[0] == '\0') {
            return -1;
        }

    }

    return 0;

}

// Check if list of waypoints is valid
int checkWaypoints(List *waypoints) {

    void *elem;
    List *waypointList = waypoints;
    ListIterator waypointIter = createIterator(waypointList);

	while ((elem = nextElement(&waypointIter)) != NULL) {

        Waypoint *tmpWpt = (Waypoint *)elem;

        // Latitude and longitude must be within correct ranges
        if ((tmpWpt->latitude > 90 || tmpWpt->latitude < -90) || (tmpWpt->longitude > 180 || tmpWpt->longitude < -180)) {
            return -1;
        }

        // Name must not be NULL
        if (tmpWpt->name == NULL) {
            return -1;
        }

        // Check other data as well
        if (checkGPXData(tmpWpt->otherData) != 0) {
            return -1;
        }

	}
    
    return 0;

}

// Check if list of rotues is valid
int checkRoutes(List *routes) {

    void *elem;
    List *routeList = routes;
    ListIterator routeIter = createIterator(routeList);

	while ((elem = nextElement(&routeIter)) != NULL) {

        Route *tmpRte = (Route *)elem;

        // Name must not be NULL
        if (tmpRte->name == NULL) {
            return -1;
        }

        // Check other data list
        if (checkGPXData(tmpRte->otherData) != 0) {
            return -1;
        }

        // Check waypoints list
        if (checkWaypoints(tmpRte->waypoints) != 0) {
            return -1;
        }

	}
    
    return 0;
    
}

// Check if track seg list is valid
int checkTrackSeg(List *trackSegs) {

    void *elem;
    List *trackSegList = trackSegs;
    ListIterator trackSegIter = createIterator(trackSegList);

    while((elem = nextElement(&trackSegIter)) != NULL) {

        TrackSegment *tmpTrkSeg = (TrackSegment *)elem;

        // Check waypoints list
        if (checkWaypoints(tmpTrkSeg->waypoints) != 0) {
            return -1;
        }

    }

    return 0;

}

// Check if tracks list is valid
int checkTracks(List *tracks) {

    void *elem;
    List *trackList = tracks;
    ListIterator trackIter = createIterator(trackList);

	while ((elem = nextElement(&trackIter)) != NULL) {
        
        Track *tmpTrk = (Track *)elem;

        // Name must not be NULL
        if (tmpTrk->name == NULL) {
            return -1;
        }

        // Check other data list
        if (checkGPXData(tmpTrk->otherData) != 0) {
            return -1;
        }

        // Check segments list
        if (checkTrackSeg(tmpTrk->segments) != 0) {
            return -1;
        }

	}
    
    return 0;

}

// Function implementing haversine formula
double haversine(double lat1, double lon1, double lat2, double lon2) {

    const int radius =  6371e3;

    double lat1InRadians = lat1 * (M_PI/180);
    double lat2InRadians = lat2 * (M_PI/180);

    double deltaLat = (lat2 - lat1) * (M_PI/180);
    double deltaLon = (lon2 - lon1) * (M_PI/180);

    double a = (sin(deltaLat/2) * sin(deltaLat/2)) + (cos(lat1InRadians) * cos(lat2InRadians) * sin(deltaLon/2) * sin(deltaLon/2));

    double c = 2 * atan2(sqrt(a), sqrt(1-a));

    return radius * c;

}

// Function to get the total distance of all waypoints in a list
float getTotalWaypointsLen (List *waypoints) {

    float total = 0.0;

    if (waypoints == NULL) {
        return total;
    }


    if (getLength(waypoints) < 2) {
        return total;
    }

    void *elem;
    List *waypointList = waypoints;
    ListIterator waypointIter = createIterator(waypointList);

    double tmpLat, tmpLon;

    int i = 0;
	while ((elem = nextElement(&waypointIter)) != NULL) {

        Waypoint *tmpWpt = (Waypoint *)elem;

        // Only if i is > 0, because the 1st iteration will contain garbage values for tmpLat and tmpLon
        if (i > 0) {
            total += haversine(tmpLat, tmpLon, tmpWpt->latitude, tmpWpt->longitude);
        }

        // Set tmpLat and tmpLon, (2nd iteration will be first to touch total)
        tmpLat = tmpWpt->latitude;
        tmpLon = tmpWpt->longitude;

        i++;

	}
    
    return total;

}

// Same as previous function except for track segments
float getTotalTrackSegLen(List *trackSegs) {

    float total = 0.0;

    if (trackSegs == NULL) {
        return total;
    }

    void *elem;
    List *trackSegList = trackSegs;
    ListIterator trackSegIter = createIterator(trackSegList);

    double tmpLat1, tmpLon1, tmpLat2, tmpLon2;

    int i = 0;
    while((elem = nextElement(&trackSegIter)) != NULL) {

        TrackSegment *tmpTrkSeg = (TrackSegment *)elem;

        total += getTotalWaypointsLen(tmpTrkSeg->waypoints);

        if (i > 0) {
            Waypoint *tmpWpt2 = getFromFront(tmpTrkSeg->waypoints);
            tmpLat2 = tmpWpt2->latitude;
            tmpLon2 = tmpWpt2->longitude;
            total += haversine(tmpLat1, tmpLon1, tmpLat2, tmpLon2);
        }

        Waypoint *tmpWpt = getFromBack(tmpTrkSeg->waypoints);
        tmpLat1 = tmpWpt->latitude;
        tmpLon1 = tmpWpt->longitude;

        i++;

    }

    return total;

}

// Dummy delete function that does nothing, for use in getRoutesBetween/getTracksBetween
void dummyDelete(void* data) {}
