#[repr(C)]
pub enum ExitCode {
    OK,
    FAILURE,
}

impl From<i8> for ExitCode {
    fn from(value: i8) -> Self {
        match value {
            0 => ExitCode::OK,
            _ => ExitCode::FAILURE,
        }
    }
}

impl std::fmt::Display for ExitCode {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match &self {
            ExitCode::OK => write!(f, "OK"),
            ExitCode::FAILURE => write!(f, "FAILURE"),
        }
    }
}

use std::ffi::c_char;

extern "C" {
    pub fn export_game(
        source_path: *const c_char,
        destination_path: *const c_char,
        game_id: *const c_char,
    ) -> ExitCode;
}
