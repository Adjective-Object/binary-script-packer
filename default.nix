let pkgs = import <nixpkgs> {};
in with pkgs; let
    
    devDependencies = [
        stdenv
        # build
        gcc
        gnumake
        cmake

        # check
        llvmPackages.clang-unwrapped # for clang-format
        cppcheck
        valgrind

        # etc
        hexcurse
        ctags
    ];
    
    dependencies = [
        
    ];

in {
    devEnv = stdenv.mkDerivation {
        name = "binscripter";
        buildInputs = devDependencies ++ dependencies;
    };
}

