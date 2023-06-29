add_rules("mode.debug", "mode.release", "mode.releasedbg")
add_cxflags("-fmacro-prefix-map=$pwd/=.")
set_languages("c99", "cxx11")

set_config("cc", "clang")
set_config("ld", "clang++")

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
-- add_includedirs("lib/include/http")
add_files("lib/*.cpp")

-- add http parser(form: https://github.com/mongrel2/mongrel2/tree/master/src/http11)
add_includedirs("packages/http11")
add_files("packages/http11/*.cpp")

-- target

-- source files are in "$ProjectDir/src"
src_targets = {
  "main",
}
-- source files are in "$ProjectDir/examples"
example_targets = {
  "echo_server"
}
-- source files are in "$ProjectDir/tests"
test_targets = {
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
  "test_http_parser",
  "test_tcp_server",
  "test_util",
}

-- iterate over all targets
function iterate_target(prefix_path ,targets)
  for i, label in pairs(targets) do
    target(label)
      set_kind("binary")
      add_files(prefix_path .. label .. ".cpp")
  end
end

iterate_target("src/", src_targets)
iterate_target("examples/", example_targets)
iterate_target("tests/", test_targets)