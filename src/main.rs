use ps2opl2iso::PS2Util;

fn main() -> () {
    let args = std::env::args().collect::<Vec<String>>();
    let option = args.get(1).expect("option not provided");

    match option.as_str() {
        "help" => usage(),
        "list" => {
          let source_path = args.get(2).expect("source_path not provided");
          println!("{:#?}", PS2Util::list(source_path.to_owned()));
        },
        "export" => {
            let source_path = args.get(2).expect("source_path not provided");
            let target_path = args.get(3).expect("target_path not provided");

            let game_id = args.get(4);
            match game_id {
                Some(game_id) => {
                    let program = PS2Util::new(source_path.to_owned(), target_path.to_owned());
                    match program.find_game(game_id) {
                        Some(game) => program.export(&game),
                        None => panic!("Game not found for {game_id} in path {source_path}"),
                    }
                }
                None => {
                    let games = PS2Util::list(source_path.to_owned());
                    let program = PS2Util::new(source_path.to_owned(), target_path.to_owned());
                    for game in games {
                        program.export(&game);
                    }
                }
            };
        }
        _ => panic!("Invalid option!"),
    }
}

fn usage() -> () {
    println!("ps2opl2iso usage:\n");
    println!("list <source_device_path>: list all games on the device");
    println!("export <source_device_path> <target_device_path>: export all games from the source device path to the target");
    println!("export <source_device_path> <target_device_path> <game_id>: export a single game matching the <game_id> from the source device path to the target");
}
