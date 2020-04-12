# CppDebugServer
 
The goal of CppDebugServer is to provide an easy, quick-setup Debug Render application for use with C++ applications.

CppDebugServer gives users control to render data in a way that suits their application and allows them to easily hook into their application data with minimal/no need to modifiy existing code. 

# Getting Started:

There are a few interesting concepts involved in CppDebugServer.
## In C++:
* Run the server: 
    Before entering the main loop, use the provided macro (#include <server.h>): RUN_DEBUG_SERVER(port), specifying the port you want to run the server on.

* For all data that needs to be rendered,
    add in a global scope:
        DEFINE_<ContentType>_REQUEST_HANDLER 
        Currently Supported <ContentType>s are ARRAY and VECTOR

    Before entering the main loop, use the provided macro (#include <server.h>): 
    ADD_REQUEST_HANDLER(id) where id is the unique id you have given to this handler. This id is important, it's what you'll use on the js side to request this data.

## Config: 
* Syntax:
    * Each setting should come as the first thing on each line, followed by exactly one space and then the value for the setting.

    * In cases where a setting is a list of values, these should follow the following syntax: <br />
    VariableName { <br />
        value0 <br />
        value1 <br />
    } <br />

    Every CppDebugServer project requires a server_config file.
    This should be placed in your bin directory or wherever your executable will be run from.

    There are a few parameters you can specify in this file but first, I will concentrate on the most important.

    WEB_FILES_ROOT: This is where all of your user scripts will live and the location that will be used to search for all files that are requested by the client.

    USER_SCRIPTS: This is how you do your rendering. All rendering in CppDebugServer is handled using Three.js to render in-browser. So in this variable is where you should

## In Js:
All of your rendering should be done in user scripts that you specify using the USER_SCRIPTS parameter in your server_config file.

Some functionality is included by default:

* DataRequester:
    * Use this object to make requests for data from the server.
    Just creating one of these with an associated data ID and handler function will result in the request being made each js animation frame and so keeps the data up-to-date in real-time.

    * The rendering is then up to you in your handler function but Three.js comes with a whole load of documentation and examples to help you get started.
