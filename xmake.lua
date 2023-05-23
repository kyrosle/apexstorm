add_rules("mode.debug", "mode.release")

set_targetdir("bin")
target("apexstorm")
    set_kind("binary")
    add_files("src/*.cpp")
    add_includedirs("lib")
