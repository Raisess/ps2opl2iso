pub struct Device {
    path: String,
}

impl Device {
    pub fn new(path: String) -> Self {
        Device { path }
    }

    pub fn path(&self) -> String {
        self.path.clone()
    }
}
