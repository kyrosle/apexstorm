add_rules("mode.debug", "mode.release")

set_targetdir("bin")

add_includedirs("lib/include")
add_files("lib/*.cpp")

target("apexstorm")
  set_kind("binary")
  add_files("src/*.cpp")

target("test")
  set_kind("binary")
  add_files("tests/*.cpp")