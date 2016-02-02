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
    ];
    
    dependencies = [
        
    ];

in {
    devEnv = stdenv.mkDerivation {
        name = "binscripter-dev";
        buildInputs = devDependencies ++ dependencies;
    };
}

