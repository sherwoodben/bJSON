--[[
##########################################################################################
    "Ultimate" C++ PREMAKE BUILD SCRIPT
##########################################################################################
Author:   Benjamin Sherwood
Version:  0.1.0

  - Provides a project scaffold and configuration action
  - Meant to ease creating new C++ projects
  - Also provides an actual implementation of the clean action
  - Capable of generating tests (with a copy of "bUnitTests.h")

  Sure, this _could_ be split into smaller files which are all included in this file,
  but at the end of the day it's easier to just keep everything in one place. Besides,
  it's not like this file is going to be changed too much now that it's working just
  about how I want it to!

--]]

--[[
##########################################################################################
    USAGE/INSTRUCTIONS -- what's going on in here?
##########################################################################################

  This script has been developed to (hopefully) simplify creating projects in the future!

  For additional premake actions/options which have been added, see the section below
  the "BUILDING" section.

  One of the additional actions which has been implemented is a configuration action--
  running premake (targeting this file) with the "configure" action will begin a process
  which prompts the user for "configuration variables", then store them such that the
  next time the project is built the correct values are reflected. The "configure" action
  must be run before any "build" action.

  The "configure" action does not implement filtering functions or the example project
  generation function at this time. To change the default behavior of these functions,
  one must make manual edits to this file in the section below.

##########################################################################################
    "PROJECT SPECIFIC" functions-- update these to reflect the current project
##########################################################################################
--]]

function workspace_filters()
  --[[ Would look something like this...
      filter "configurations:FILTER"
        defines{"SOMETHING",}
      filter{}
    ]]

  --[[ Or something like this...
      filter "platforms:FILTER"
        defines{"SOMETHING_ELSE",}
      filter{}
    ]]
end

function project_filters()
  --[[ Would look something like this...
      filter "configurations:FILTER"
        defines{"SOMETHING",}
      filter{}
    ]]

  --[[ Or something like this...
      filter "platforms:FILTER"
        defines{"SOMETHING_ELSE",}
      filter{}
    ]]
end

function make_examples()
  --[[ Would look something like this...
      project "example1"
        set_project_defaults()
        kind "PROJECT TYPE"

        files{"../examples/example1/**.*"}

      project "example2"
        set_project_defaults()
        kind "PROJECT TYPE"

        files{"../examples/example2/**.*"}

      ...
    ]]
end

--[[
##########################################################################################
    "CONTEXT VARIABLES" -- useful for getting/setting/manipulating context values
##########################################################################################
--]]

-- this is the information associated with each context variable to be set by the user.
-- Each "struct" within the table contains the value's name, its value (which begins as
-- nil), the prompt for setting the value, a flag which states if the value is optional,
-- and a default value.
--
-- values (and defaults) are stored as strings, since the data in this table will be
-- loaded from/encoded to JSON. Values will be "decoded" before their use. Also, strings
-- are really easy to get as input from the user!
contextVariablesMetadata = {
  { -- application (windowed/console), library (static/shared)
    name = "projectType",
    value,
    prompt = [[
What should the project build?
  - Value can be 'App' or 'ConsoleApp' or 'WindowedApp' or 'StaticLib' or
    'SharedLib'.
  - For 'App' types, a console will be available in 'Debug' mode and not in
    'Release' mode.
  - For 'ConsoleApp' and 'WindowedApp' types, the presence of a console is not
    dependent on 'Debug' or 'Release' mode.]],
    optional = false,
    default = "app" },
  { -- preprocessor definitions for debug mode
    name = "debugDefinitions",
    value,
    prompt = [[
Provide preprocessor definitions for 'Debug' mode (other than 'DEBUG').
  - Values must be separated by a comma.]],
    optional = true,
    default = "" },
  { -- preprocessor definitions for release mode
    name = "releaseDefinitions",
    value,
    prompt = [[
Provide preprocessor definitions for 'Release' mode (other than 'RELEASE').
  - Values must be separated by a comma.]],
    optional = true,
    default = "" },
  { -- additional configurations
    name = "additionalConfigurations",
    value,
    prompt = [[
Provide additional configurations (other than 'Debug'/'Release').
  - Values must be separated by a comma.]],
    optional = true,
    default = "" },
  { -- additional platforms
    name = "additionalPlatforms",
    value,
    prompt = [[
Provide additional platforms (other than 'Windows').
  - Values must be separated by a comma.]],
    optional = true, 
    default = "" },
  { -- additional projects to build
    name = "additionalProjectDirs",
    value,
    prompt = [[
Provide '/premake/' directories for additional projects to build.
  - Values must be separated by a comma.]],
    optional = true,
    default = "" },
  { -- libraries to link
    name = "libraries",
    value,
    prompt = [[
Provide the names of the libraries to link.
  - Values must be separated by a comma.]],
    optional = true,
    default = "" },
  { -- library include directories
    name = "libraryIncludeDirs",
    value,
    prompt = [[
Provide the library include directories.
  - Values must be separated by a comma.
  - Values can take advantage of '*' (and '**') wildcard matching.]],
    optional = true,
    default = "" },
  { -- library binaries
    name = "libraryBinaryDirs",
    value,
    prompt = [[
Provide the directories for the library binaries.
  - Values must be separated by a comma.
  - Values can take advantage of '*' (and '**') wildcard matching.]],
    optional = true,
    default = "" },
  { -- prebuild commands
    name = "prebuildCommands",
    value,
    prompt = [[
Provide a list of commands to run as prebuild commands.
  - Values must be separated by a comma.
  - Values can take advantage of premake \"tokens\" (but might require wrapping in '%%[]').]],
    optional = true,
    default = "" },
  { -- postbuild commands
    name = "postbuildCommands",
    value,
    prompt = [[
Provide a list of commands to run as postbuild commands.
  - Values must be separated by a comma.
  - Values can take advantage of premake \"tokens\" (but might require wrapping in '%%[]').]],
    optional = true,
    default = "" },
  { -- test generation flag
    name = "buildTests",
    value,
    prompt = [[
Generate tests?
  - Set to true to build a test application and false to not.

  Note:
  - If true, this will attempt to download a copy of the latest version of the
    bUnitTests ("single-file" header) framework to the project's /test/include
    directory. The copy of bUnitTests.h can be updated to the latest version at
    any time by deleting the local copy and rebuilding the project with premake
    (with tests enabled).]],
    optional = true,
    default = "true" },
  { -- example generation flag
    name = "buildExamples",
    value,
    prompt = [[
Generate examples?
  - Set to true to build example applications and false to not.]],
    optional = true,
    default = "false" },
}

-- alias the long name to something a bit shorter
local cvMetadata = contextVariablesMetadata

--[[
  a table to store the "interpreted" values in; no longer are the values necessarily
  strings (now they might be arrays of strings). To keep things simple, we will only
  use strings as the "type" (while allowing arrays of strings). So, if we want to check
  if a value is set to true, we would compare the value's string value to "true" i.e

     if (contextVariables[variable] == "true") then ... <-- good! treating the value
                                                             of contextVariables[variable]
                                                             as a string!
  instead of

     if (contextVariables[variable]) then ... <-- bad! contextVariables[variable] might
                                                   be "false", but the if statement returns true!

  Of course, the above assumes the variable is known to be not an array. If the variable is
  a single string or an array of strings should hopefully be clear from context!
--]]
contextVariables = {
  workspaceName,
  projectName,
  projectType,
  debugDefinitions,
  releaseDefinitions,
  additionalConfigurations,
  additionalPlatforms,
  additionalProjectDirs,
  libraries,
  libraryIncludeDirs,
  libraryBinaryDirs,
  prebuildCommands,
  postbuildCommands,
}

-- alias the long name to something a bit shorter
local cv = contextVariables

-- gets the context values from the "config.json" file (and the current working directory)
--
-- @return a value on failure to read/parse the json file and nil on success
function get_values_from_json()
  -- build the project name from the current working directory
  local PROJECT_NAME = os.getcwd()
  -- get the name of the directory above this directory...
  PROJECT_NAME = string.gsub(PROJECT_NAME, ".*/([^/]+)/[^/]+/?$", "%1")
  
--[[
  let's explain that pattern because WHAT is ".*/([^/]+)/[^/]+/?$" ?

  anchor at the end: $
  maybe there's a path separator at the end: /? (add to) $
  get the folder name (every character that isn't a path separator): /[^/]+ (add to) /?$
  capture the next higher folder name: /([^/]+) (add to) /[^/]+/?$
  match any character before what we've captured: .* (add to) /([^/]+)/[^/]+/?$

  that's it ! we now have ".*/([^/]+)/[^/]+/?$" and capture 1 (%1) returns the name of the
  directory above the premake directory this file is found in. Using gsub, we replace the
  entire target string with the captured value, so the PROJECT_NAME only holds the value we
  captured!
--]]

  -- get the values from the "config.json" file...
  local fromJSON = io.readfile("config.json")

  -- if we failed to load the json file in configuration mode that's okay! but if we're in
  -- a "build" action we need to have the file to load from
  if (_ACTION ~= "configure" and (fromJSON == nil or fromJSON == "")) then
    local failMsg = [[
Failed to read values from "config.json".
No solution/workspace will be generated.
Be sure to run the 'configure' action prior to a 'build' action!]]
    printf(failMsg)
    return false
  end

  -- convert the json to a lua table
  local toTable = json.decode(fromJSON)

--[[
  - for every pair in the lua table, set the contextVariable's value at
      the index provided by the key to the table's value. It is at this point that we need to
      take into account the value in the json file may represent an array of values.

  - we assume the separator is a comma and explode it into parts separated by a comma
  - we prepare an array to fill with the parts of the exploded string
  - we prepare a variable to keep track of the number of elements in the array
  - for every part in the list of exploded strings, insert the part into the new array at
        the current index
  - then once all values have been pushed to the array, if the size is greater than one the
        value in the contextVariable "struct" is set to the entire array. If the size is just
        1, the value in the contextVarable "struct" is set to the single value
--]]

  for k,v in pairs(toTable ~= nil and toTable or {}) do
    local parts = string.explode(v, "%s*,%s*") -- explode into parts
    local arr = {} -- prepare the array to assign to the cv value
    local size = 0 -- the number of values in the new array
    for idx, part in ipairs(parts) do
      arr[idx] = part -- set the current index of the array to the string
      size = size + 1 -- increment the size counter
    end 
    cv[k] = size > 1 and arr or arr[1] -- assign the value in the cv "struct"
  end
  
  -- a few extra values which do not come from the config file...
  cv["workspaceName"] = PROJECT_NAME
  cv["projectName"] = PROJECT_NAME
end

--[[
##########################################################################################
    "UTILITY" functions for configuring projects
##########################################################################################
--]]

function set_project_defaults()
  -- Set the project location
  location "%{wks.location}/../build/"

  -- Set the output/obj/debug directories
  targetdir "%{prj.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"
  objdir "%{prj.location}/obj/%{cfg.platform}/%{cfg.buildcfg}"
  debugdir "%{prj.location}/bin/%{cfg.platform}/%{cfg.buildcfg}"

  -- The language is C++
  language "C++"
  -- 64 bit architecture
  architecture "x64"
  -- Default to C++20
  cppdialect "C++20"
  -- Default to UTF-8 encoding
  characterset "Unicode"
  -- ...and add /utf-8 to the compiler
  buildoptions
  {
    "/utf-8",
  }

  -- staticruntime/runtime configurations
  staticruntime "on"
  filter "configurations:Debug*"
    runtime "Debug"
  filter "configurations:Release*"
    runtime "Release"
  filter {}

  -- user provided filters
  project_filters()
  filter {}

  -- all projects (in this solution/workspace) will include the main project's
  -- include directory and the test include directory
  includedirs
  {
    "../include/",
    "../include/*/",
    "../tests/include/",
  }

  -- all projects (in this solution/workspace) will include the library include directories
  table.foreachi(cv.libraryIncludeDirs, includedirs)
  -- includedirs(contextVariables.libraryIncludeDirs) -- bad!!!

  -- all projects (in this solution/workspace) will include the files in the /include/ and /src/
  -- directories by default
  files
  {
    "../include/**.*",
    "../src/**.*",
    -- ADD additional files as needed (resources/assets, etc.) in the specific project section
  }

  -- all projects (in this solution/workspace) need to know where to find library binaries
  table.foreachi(cv.libraryBinaryDirs, libdirs)
  -- libdirs(contextVariables.libraryBinaryDirs) -- bad!!!

  -- all projects (in this solution/workspace) need to link the libraries
  table.foreachi(cv.libraries, links)
  -- links(contextVariables.libraries) -- bad!!!
end

-- Generates a project for a unit testing application. The application builds to a console app
-- and expects to "live" in a directory structure which matches the following:
--
-- /mainProject/
--    /build/ <--- will contain the main project's executable and the test executable
--    /include/ <--- the main project's header files (can include tests here-- at least I think)
--    /premake/ <--- this main premake5.lua file lives here
--    /src/ <--- the main project's source files (can include tests here)
--    /tests/
--        /include/ <--- only contains "UnitTests.h" (do not add any other files here)
--
-- To make sure this works, ensure the directory of the main project matches the above
function make_test_project ()
  -- attempt to download the test include file from github (if it doesn't already exist):
  if (os.isfile("../tests/include/bUnitTests.h") ~= true) then
    -- Create the tests directory (if it doesn't exist)
    os.mkdir("../tests/")
    os.mkdir("../tests/include/")
    -- download the file
    http.download(
      "https://raw.githubusercontent.com/sherwoodben/bUnitTests/main/include/bUnitTests.h",
      "../tests/include/bUnitTests.h",
      {
        timeout = 5,
        progress = function(total, current)
          printf("Downloading tests header (%i%%)...", math.floor(100 * (current / total)))
        end
      }
    )
  end
  -- Create the testing project
  project "tests"
    set_project_defaults()
    kind "ConsoleApp"

    -- set a preprocessor macro so we know we're building tests!
    defines{"bBUILD_TESTS",}

    -- Set the project specific files
    files{"../tests/**.*",}
end

--[[
##########################################################################################
    WORKSPACE configuration function
##########################################################################################

  makes the workspace/solution for the projects which will be generated by this file

  reads values from the contextVariables table, so the table must be populated before
  calling this function. At the very least, the contextVariables.projectName must NOT
  be empty. The contextVariables table is typically populated by a call to the
  get_values_from_json() function
--]]
function make_workspace()
  -- Generate a workspace with the desired name
  workspace(cv.projectName)
  
  -- Set the start project to the main project
  startproject(cv.projectName)
  
  -- Set the available configurations
  configurations{"Debug", "Release",}
  table.foreachi(cv.additionalConfigurations, configurations) -- from config value
  
  -- Set the available platforms
  platforms{"Windows",}
  table.foreachi(cv.additionalPlatforms, platforms) -- from config value
  
  -- Debug preprocessor definitions
  filter "configurations:Debug*"
    defines{"DEBUG",}
    table.foreachi(cv.debugDefinitions, defines) -- from config value
  -- Release preprocessor definitions
  filter "configurations:Release*"
    defines{"RELEASE",}
    table.foreachi(cv.releaseDefinitions, defines) -- from config value
  filter{}

  -- user provided filters
  workspace_filters()
  filter {}
  
  -- Build with symbols (default "On")
  symbols "On"
end

--[[
##########################################################################################
  MAIN PROJECT configuration function
##########################################################################################

    makes the main project for the workspace/solution

    requires that the contextVariables.projectName value has been initialized (it is not
    null), typically through a call to the get_values_from_json() function
    
    if the "generate tests" config flag has been set, the main project links the test
    project, which generates a build dependency so that the test application is built as
    part of the main application build step. Additionally if in (any) release mode, a
    prebuild command to run the test application is included

    for now the main project defaults to a console application in debug mode and a
    windowed application in release mode. After this file is "cleaned" and any bugs are
    ironed out, a configuration value can be added to reflect the type of project to build
    (console/windowed application, static library, shared library)  
--]]
function make_main_project()

  project(cv.projectName) -- from config value
    -- run the function which sets project defaults
    set_project_defaults()

    -- set the project type based on the config value
    if (string.lower(cv.projectType) == "consoleapp") then kind "ConsoleApp"
    elseif (string.lower(cv.projectType) == "windowedapp") then kind "WindowedApp"
    elseif (string.lower(cv.projectType) == "staticlib") then kind "StaticLib"
    elseif (string.lower(cv.projectType) == "sharedlib") then kind "SharedLib"
    elseif (string.lower(cv.projectType) == "app") then
      filter "configurations:Debug*"
        kind "ConsoleApp"
      filter "configurations:Release*"
        kind "WindowedApp"
      filter{}
    end
    if (string.find(string.lower(cv.projectType), "lib")) then defines{ "bNO_ENTRY_POINT", } end

    -- only do the following if we're generating tests...
    if (string.lower(cv.buildTests) == "true") then
      -- the tests project is an application, so linking it just creates a build dependency
      links {"tests",}
  
      -- only run the tests automatically in release mode
      -- only build documentation automatically in release mode
      filter "configurations:Release*"
        prebuildmessage "Running tests."
        prebuildcommands
        {
          "cd %[%{prj.location}bin/%{cfg.platform}/%{cfg.buildcfg}]",
          "tests.exe"
        }
        postbuildmessage "Building documentation."
        postbuildcommands
        {
          "cd %[%{prj.location}../tools]",
          "Build_Documentation_HTML.bat"
        }
      filter{}
    end

    -- add pre/post build commands
    local interpret_and_add_to_prebuildcommands = function(command)
      local withReplacedTokens = string.gsub(command, "%$PLATFORM", "%{cfg.platform}")
      withReplacedTokens = string.gsub(withReplacedTokens, "%$CONFIG", "%{cfg.buildcfg}")
      prebuildCommands(withReplacedTokens)
    end
    table.foreachi(cv.prebuildCommands, prebuildcommands) -- from config value

    local interpret_and_add_to_postbuildcommands = function(command)
      local withReplacedTokens = string.gsub(command, "%$PLATFORM", "%{cfg.platform}")
      withReplacedTokens = string.gsub(withReplacedTokens, "%$CONFIG", "%{cfg.buildcfg}")
      postbuildCommands(withReplacedTokens)
    end
    table.foreachi(cv.postbuildCommands, postbuildcommands) -- from config value
end

--[[
##########################################################################################
    BUILDING -- this function should be run for any "build" action!
##########################################################################################
      
    makes the workspace/solution and all of the projects which are included in the
    workspace/solution

    this function calls get_values_from_json() to populate the contextVariables table,
    but if an error is encountered in retrieving the values then the workspace/solution
    and any projects are NOT created.
--]]
function build_workspace_and_projects()
  -- first, make sure the "contextVariables" struct is "initialized"; fails if the config.json
  -- file is not found or cannot be decoded
  local err = get_values_from_json()
  if (err ~= nil) then return end

  -- first, set up the workspace
  make_workspace()
  
  -- then generate the main project
  make_main_project()

  -- make any other projects (i.e. dependencies) we build
  group "Also Build"
  table.foreachi(cv.additionalProjectDirs, include) -- from config value

  -- make the test application (if we're building tests)
  group "Tests"
  if (string.lower(cv.buildTests) == "true") then make_test_project() end
  
  -- make the examples (if we're building examples)
  group "Examples"
  if (string.lower(cv.buildExamples) == "true") then make_examples() end
end

--[[
##########################################################################################
    "configure" helper functions
##########################################################################################
  
    this file is meant to be "portable" in the sense that it _shouldn't_ require any
    external files. Maybe "self contained" is a better word? Sure, we could put all of
    these into a module/library but at the end of the day making them available here
    worksjust as well.
--]]

-- gets a value from user input, ensuring the result is not empty if the value is not
-- optional
--
-- @param optional describes if the value is optional or required (true means
-- an empty value is acceptable, false means a value must be provided)
function get_value(optional)
  local val
    while (val == nil or (not optional and val == "")) do
      if (optional) then
        printf("Enter a value or press 'enter' to continue with the default/current value.")
      else
        printf("Enter a value.")
      end
      val = io.read()
    end
  return val
end

-- runs the configuration process for a contextVariableMetadata entry. Prints information
-- about the value (if it's optional or not, its default value, its current value, etc.) and
-- gets a value from the user to update the stored value to
--
-- @param v the "value 'struct'" to configure (an object in the contextValuesMetadata array)
function configure_value(v)
  -- print some information about the current value (name and if it's optional or required)
  -- as well as the prompt and the default value
  printf("value: %s -- %s", v.name, v.optional and "[OPTIONAL]" or "[REQUIRED]")
  printf(v.prompt)
  printf("(default value: %s)", v.default == "" and "(empty)" or v.default)

  -- print the current value... if it exists! If this is the first time running the "configure"
  -- action and the first pass (i.e. no "retry"), these values will be nil and we won't print
  -- anything yet
  local currentVal = ""
  if (v.value ~= nil or cv[v.name] ~= nil) then
    -- first, figure out what exactly our current value is:

    -- if the value in the value "struct" is already set, use that value. This corresponds
    -- to a value set the first time and then the "retry" option is used to tweak values which
    -- were entered. Has "priority" compared to the value in the (on disk) "config.json" file
    if (v.value ~= nil) then
      currentVal = v.value
    -- otherwise, the value which was loaded from the "config.json" (on disk) is used (i.e a project
    -- is being reconfigured after building or any other time the "configure" action is run)
    else
      -- the value stored in the json file is a list of comma separated strings (or a single string)
      -- for ALL values, which makes this easy... just convert the contextVariables table (array)
      -- into a string which uses ", " (comma and a space) as a separator using the table.implode
      -- function. If the size of the table is just 1, just that string is returned!
      currentVal = cv[v.name][2] ~= nil and table.implode(cv[v.name], "", "", ", ") or cv[v.name]
    end

    --actually print the current value
    printf("(current value: %s)", currentVal ~= "" and currentVal or "(empty)") 
  end

  -- now get the value from the user. The value passed to the function is a flag representing if the
  -- value is "optional" or not. If the value exists, then we can tell the get_value function that
  -- the value is optional so that the user doesn't need to overwrite it! If the value doesn't exist,
  -- then the flag depends on if the value is optional or not.
  local val = get_value(v.value ~= nil or v.optional)
  
  -- if the value the user provided is empty...
  if (val == "") then
    -- set the value to the current value IF it exists, or the default value
    v.value = currentVal ~= "" and currentVal or v.default
  -- this lets the user reset the value to the default value...
  elseif (val == "--default") then
  v.value = v.default
  -- if the value the user provided is NOT empty (or "--default"), simply update the value
  else
    v.value = val
  end

  -- print the new value, then wait for user input
  printf("%s is %s", v.name, (v.value ~= "" and v.value or "(empty)"))
  printf("\nPress 'enter' to continue.")
  io.read()
end

-- runs the configure_value(...) function for all value "structs" in the contextVariablesMetaData
-- table
function configure_all_values()
  local numToConfigure = 0
  for _, _ in ipairs(cvMetadata) do numToConfigure = numToConfigure + 1 end
  for idx, valStruct in ipairs(cvMetadata) do
    local clearScreen = os.execute("cls") or os.execute("clear")
    printf("Configuring %s (%i/%i)", cv.projectName, idx, numToConfigure)
    configure_value(valStruct)
  end
end

-- @return true if the values are acceptable, or false if the configuration should be
-- cancelled. Can also return nil, in which case the configuration operation is retried
-- so that the values can be tweaked
 function review_values()
  clearScreen = os.execute("cls") or os.execute("clear")
  printf("REVIEW:\n")
  for idx, valStruct in ipairs(cvMetadata) do
    printf("%s is %s", valStruct.name, valStruct.value ~= "" and valStruct.value or "(empty)")
  end
  printf("\nTo confirm these values, type 'yes'. To retry, type 'retry'.")
  printf("Entering ANY OTHER text (or simply pressing 'enter') will cancel the configuration.")
  confirm = io.read()
  if (confirm == "yes" or confirm == "YES" or confirm == "y" or confirm == "Y") then
    return true
  elseif (confirm ~= "retry" or confirm == "RETRY" or confirm == "r" or confirm == "R") then
    return false
  end  
end

-- saves the values in the contextVariablesMetadata array to a "config.json"
-- the entries in the contextVariablesMetadata array have a value and name accessible at
-- the "value" and "name" indices which are stored as strings. These strings are then saved to a
-- JSON file as key/value pairs with the name string as the key and the value string as
-- the value
function save_values_to_json()
  local jsonTable = {}
  -- add each "name/value" pair to the table
  for _, valStruct in ipairs(cvMetadata) do
    jsonTable[valStruct.name] = valStruct.value
  end
  -- encode the array as json
  local asJSON, err = json.encode(jsonTable)
  if (err) then
    printf("Encoding error -- %s.")
    return
  end
  -- format the resulting string for better readability
  asJSON = string.gsub(asJSON, "{%s?", "{\n") -- newline after curly bracket
  asJSON = string.gsub(asJSON, "%s?:%s?", " : ") -- space between name/val pairs
  asJSON = string.gsub(asJSON, "(\"[^\"]+\" : \"[^\"]*\",?)", "\t%1\n") -- indent name/val pairs
  
  -- write the string to the "config.json" file and check for errors
  local good, err = os.writefile_ifnotequal(asJSON, "config.json")
  if (not good) then
    printf("File saving error -- %s.")
    return
  end

  -- "success" message:
  printf("Configuration saved as \"config.json\".")
  printf("To see changes, rebuild the workspace/solution with premake.")
end

-- the function that runs when the configure action executes
function on_configure_action()
  -- print a bunch of info, then wait for user input
  printf("For each configurable value, the system will present a prompt.")
  printf("To skip an optional value, simply press the enter key.")
  printf("You will have a chance to review these values prior to any changes taking effect.")
  printf("Press the 'enter' key to begin.")
  io.read()
  
  -- first, try to read values from an existing "config.json" file. If the file
  -- doesn't exist, that's fine because we're making it now. This also populates
  -- the contextVariables table with the "projectName" key (and associated value)
  -- which is useful for displaying project information to the screen during configuration
  get_values_from_json()

  -- configure the values, then present them and get confirmation. If no choice is made, the
  -- loop goes again, allowing for the user to "retry" or tweak entered values
  local accept = nil
  while (accept == nil) do
    configure_all_values()
    accept = review_values()
  end

  -- if the changes were not accepted (and we're not retrying/tweaking them) then cancel
  -- the configuration without saving
  if (not accept) then
    printf("Configuration cancelled. All changes discared.")
    return
  end
  
  -- now, save the files to the "config.json" file!
  save_values_to_json()
  
  printf("\nDone.\n")
end

--[[
##########################################################################################
    "ACTIONS AND OPTIONS" provide more functionality to premake!
##########################################################################################
--]]

-- an actual implementation of the "clean" action
--
-- triggered by "clean", it removes the entire '/build/' directory and any file generated
-- by premake in EXCEPT for the 'config.json' file
newaction {
  trigger = "clean",
  description = "removes the '/build/' directory and any other file generated by premake (besides 'config.json')",
  execute = function() -- short enough to just define here!
    printf("\nRemoving the '/build/' directory and all its contents.")
    os.rmdir("../build/")
    printf("\nRemoving the '/docs/' directory and all its contents.")
    os.rmdir("../docs/")
    printf("\nRemoving (generated) content from the '/premake/' directory.")
    os.rmdir(".vs")
    os.remove("*.sln")
    printf("\nDone.\n")
  end,
}

-- a new action, created to configure the project with user provided values which are
-- stored in a config.json file
--
-- triggered by "configure", this action is designed to be run before any build action;
-- in fact a build action will fail unless it can find a config.json file 
newaction {
  trigger = "configure",
  description = "configures values like linked libraries, if tests should build, etc.",
  execute = on_configure_action,
}

--[[
##########################################################################################
    "ON BUILD ACTION" -- actually provide the code to run! (Not just function definitions)
##########################################################################################

  Of course, this should only run on a "build" action. Calls the
  build_workspace_and_projects() function to build the workspace/solution and all
  projects! 
--]]
if (_ACTION ~= nil) then -- got some weird errors when running "help" without this...
  if (_ACTION == "codelite" or string.find(_ACTION, "gmake") or
    string.find(_ACTION, "vs20") or _ACTION == "xcode4") then
    build_workspace_and_projects()
  end
end