add_rules("mode.debug", "mode.release")
add_cxflags("-fmacro-prefix-map=$pwd/=.")

-- add auto create `complie_commands.json` file in build/
-- so that the clangd language server can provide correct instructions.
-- including the package headers.
add_rules("plugin.compile_commands.autoupdate", {outputdir = "build"})

-- add package: `yaml-cpp`
add_requires("yaml-cpp")
add_packages("yaml-cpp")

-- set binrary executable file output directory
set_targetdir("bin")

-- include self includes
add_includedirs("lib/include")
add_files("lib/*.cpp")

-- main target
target("apexstorm")
  set_kind("binary")
  add_files("src/*.cpp")

-- test log target
target("test_log")
  set_kind("binary")
  add_files("tests/test_log.cpp")

-- test config target
target("test_config")
  set_kind("binary")
  add_files("tests/test_config.cpp")

-- test thread target
target("test_thread")
  set_kind("binary")
  add_files("tests/test_thread.cpp")

-- test fiber target
target("test_fiber")
  set_kind("binary")
  add_files("tests/test_fiber.cpp")

-- test scheduler target
target("test_scheduler")
  set_kind("binary")
  add_files("tests/test_scheduler.cpp")

-- test iomanager target
target("test_iomanager")
  set_kind("binary")
  add_files("tests/test_iomanager.cpp")

-- test util target
target("test_util")
  set_kind("binary")
  add_files("tests/test_util.cpp")