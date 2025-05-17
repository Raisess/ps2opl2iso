#[derive(Debug)]
pub struct Game {
    id: String,
    name: String,
}

impl Game {
    pub fn new(id: String, name: String) -> Self {
        Game { id, name }
    }

    pub fn id(&self) -> String {
        self.id.clone()
    }

    pub fn name(&self) -> String {
        self.name.clone()
    }
}

impl std::fmt::Display for Game {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{} {}", self.id, self.name)
    }
}
