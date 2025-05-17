fn main() -> () {
    cc::Build::new()
        .file("src/external/opl2iso/opl2iso.c")
        .compile("opl2iso");
}
