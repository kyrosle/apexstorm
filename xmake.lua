add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_cxflags("-fmacro-prefix-map=$pwd/=.")
set_languages("c99", "cxx11")

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

-- add http parser(form: https://github.com/mongrel2/mongrel2/tree/master/src/http11)
add_includedirs("packages/http11")
add_files("packages/http11/*.cpp")

-- target
-- `main` in src/main.cpp
-- others in tests/*.cpp
targets = {
  "main",
  "test_log",
  "test_config",
  "test_thread",
  "test_fiber",
  "test_scheduler",
  "test_iomanager",
  "test_timer",
  "test_hook",
  "test_address",
  "test_socket",
  "test_bytearray",
  "test_http",
  "test_util",
}

-- iterate over all targets
for i, label in pairs(targets) do
  if (label == "main")
  then
    target("main")
      set_kind("binary")
      add_files("src/*.cpp")
  else
    target(label)
      set_kind("binary")
      add_files("tests/" .. label .. ".cpp")
  end
end