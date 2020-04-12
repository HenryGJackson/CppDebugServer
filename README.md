# CppDebugServer
 
The goal of CppDebugServer is to provide an easy, quick-setup Debug Render application for use with C++ applications.

CppDebugServer gives users control to render data in a way that suits their application and allows them to easily hook into their application data with minimal/no need to modifiy existing code. 

# Getting Started:

There are a few interesting concepts involved in CppDebugServer.
## In C++:
* #include <server.h>

* Run the server: <br/>
    * RUN_DEBUG_SERVER(port): <br/>
    At some point in the start-up of your application, before entering the main loop, use this macro, specifying the port you want to run the server on.

* Define Request Handlers: <br/>
These are the the functions that will be called by the server when a data request is made. You can define them yourself following the required syntax and rules. Or, helper macros are provided to make this easier.
     * DEFINE_ARRAY_REQUEST_HANDLER(ptr, count, dataId): <br/> 
     takes a pointer to the first element of an array, the number of elements and the id that should be used to identify this array.
     * DEFINE_VECTOR_REQUEST_HANDLER(vector, dataId): <br/> 
     works just like DEFINE_ARRAY_REQUEST_HANDLER but instead takes a std::vector (or any other object that implements the data() function).

* Register Request Handlers: <br/>
     * ADD_REQUEST_HANDLER(dataId): <br/>
     is used to register a request hanlder with the server. Now, when it gets a data request, it will call your handler, which should use the dataId to check whether it is the correct handler for the request.

## Config: 
Every CppDebugServer project requires a server_config file.
This should be named "server_config" with no extension and placed in your bin directory or wherever your executable will be run from.
    
* Syntax:
    * Each setting should come as the first thing on each line, followed by exactly one space and then the value for the setting.

    * In cases where a setting is a list of values, these should follow the following syntax: <br />
     ```
     VariableName {
         value0
         value1
     }
     ```

There are a few parameters you can specify in this file but first, I will concentrate on the most important.
* Parameters:

    * WEB_FILES_ROOT: This is where all of your user scripts will live and the location that will be used to search for all files that are requested by the client.

    * USER_SCRIPTS: This is how you do your rendering. All rendering in CppDebugServer is handled using Three.js to render in-browser. So in this variable is where you should

## In Js:
All of your rendering should be done in user scripts that you specify using the USER_SCRIPTS parameter in your server_config file.

Some functionality is included by default:

* DataRequester:
    * Use this object to make requests for data from the server.
    Just creating one of these with an associated data ID and handler function will result in the request being made each js animation frame and so keeps the data up-to-date in real-time.

    * The rendering is then up to you in your handler function but Three.js comes with a whole load of documentation and examples to help you get started.
