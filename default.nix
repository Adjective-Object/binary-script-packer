let pkgs = import <nixpkgs> {};
in with pkgs; let
    
    devDependencies = [
        stdenv
        # build
        gcc
        gnumake

        # check
        llvmPackages.clang-unwrapped # for clang-format
        cppcheck
        valgrind

        # etc
        hexcurse
    ];
    
    dependencies = [
        
    ];

in {
    devEnv = stdenv.mkDerivation {
        name = "binscripter";
        buildInputs = devDependencies ++ dependencies;
    };
}

