mod device;
mod external;
mod game;

use device::Device;
use external::opl2iso::{export_game, ExitCode};
use game::Game;

pub struct PS2Util {
    source: Device,
    target: Device,
}

impl PS2Util {
    pub fn new(source_path: String, target_path: String) -> Self {
        PS2Util {
            source: Device::new(source_path),
            target: Device::new(target_path),
        }
    }

    pub fn list(source_path: String) -> Vec<Game> {
        let mut games: Vec<Game> = Vec::new();
        let file = std::fs::read(format!("{}/ul.cfg", source_path));
        match file {
            Err(_) => panic!("Failed to open {}/ul.cfg", source_path),
            Ok(content) => {
                let mut offset = 0;
                let mut size = content.len() / 64;
                while size > 0 {
                    let name = content.get(offset..offset + 31).unwrap();
                    let id = content.get(offset + 35..offset + 46).unwrap();
                    games.push(Game::new(
                        String::from_utf8_lossy(id).to_string(),
                        String::from_utf8_lossy(name).replace("\0", ""),
                    ));
                    size -= 1;
                    offset += 64;
                }
            }
        }

        games
    }

    pub fn find_game(&self, id: &String) -> Option<Game> {
        let games = Self::list(self.source.path());
        games.into_iter().find(|game| game.id().eq(id))
    }

    pub fn export(&self, game: &Game) -> () {
        unsafe {
            #[allow(temporary_cstring_as_ptr)]
            match export_game(
                std::ffi::CString::new(self.source.path()).unwrap().as_ptr(),
                std::ffi::CString::new(self.target.path()).unwrap().as_ptr(),
                std::ffi::CString::new(game.id()).unwrap().as_ptr(),
            ) {
                ExitCode::OK => println!("Successfully exported!"),
                ExitCode::FAILURE => println!("Failed to export!"),
            }
        }
    }
}
