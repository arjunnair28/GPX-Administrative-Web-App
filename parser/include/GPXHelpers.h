#ifndef GPXHELPERS_H
#define GPXHELPERS_H

#include <ctype.h>
#include "GPXParser.h"

/** Used the same looping format used in the file found at: http://www.xmlsoft.org/examples/tree1.c 
 *  in order to parse the tree. Also used the edited version provided in the file libXmlExample.c */

// Function to add a waypoint(or similar e.g. route, trackpt) into a given list
void insertWaypoints(xmlNode *cur_node, Waypoint *tmpWpt, List *listToInsertInto);

// Function to recursively read the tree returned by the XML parser and change the GPXdoc accordingly
void recursiveReader(xmlNode *a_node, GPXdoc *docToEdit);

int addGPXDataChildren(List *otherData, xmlNode *parentNode);

int addWaypointChildren(List *waypoints, xmlNode *parentNode, char *type);

int addRouteChildren(List *routes, xmlNode *parentNode);

int addTrackSegChildren(List *trackSegs, xmlNode *parentNode);

int addTrackChildren(List *tracks, xmlNode *parentNode);

int checkGPXData(List *otherData);

int checkWaypoints(List *waypoints);

int checkRoutes(List *routes);

int checkTrackSeg(List *trackSegs);

int checkTracks(List *tracks);

xmlDoc *gpxDocToXMLDoc(GPXdoc *doc);

double haversine(double lat1, double lon1, double lat2, double lon2);

float getTotalWaypointsLen (List *waypoints);

float getTotalTrackSegLen(List *trackSegs);

void dummyDelete(void* data);

#endif
