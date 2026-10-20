// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! The menu bar (macOS) or notification area (Windows) icon: a dot in the
//! color of the lamp, a short menu, and the click that opens the popup.

use disled_protocol::LampColor;
use tauri::image::Image;
use tauri::menu::{Menu, MenuItem, PredefinedMenuItem};
use tauri::tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent};
use tauri::{AppHandle, Runtime};

use crate::windows;

const TRAY_ID: &str = "disled";
const SIZE: u32 = 32;
const RADIUS: f32 = 12.0;

/// What the icon shows.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Look {
    NoLamp,
    Off,
    On(LampColor),
}

impl Look {
    pub fn tooltip(self) -> String {
        match self {
            Look::NoLamp => "DISLED: no lamp".into(),
            Look::Off => "DISLED: off".into(),
            Look::On(color) => format!("DISLED: {}", color_name(color)),
        }
    }
}

fn color_name(color: LampColor) -> &'static str {
    match color {
        LampColor::Green => "green",
        LampColor::Orange => "orange",
        LampColor::Red => "red",
        LampColor::WarmWhite => "warm white",
    }
}

fn color_rgb(color: LampColor) -> [u8; 3] {
    match color {
        LampColor::Green => [0x2e, 0xcc, 0x71],
        LampColor::Orange => [0xf3, 0x9c, 0x12],
        LampColor::Red => [0xe7, 0x4c, 0x3c],
        LampColor::WarmWhite => [0xff, 0xe2, 0xb8],
    }
}

/// Coverage (0..1) of a pixel by a disc of radius `r`, antialiased over one
/// pixel.
fn disc(distance: f32, r: f32) -> f32 {
    (r + 0.5 - distance).clamp(0.0, 1.0)
}

/// Renders the icon: a dot in the lamp color with a darker rim (so warm
/// white shows on a light menu bar), a grey ring when the lamp is off, a
/// faint ring when no lamp is connected.
pub fn render(look: Look) -> Image<'static> {
    let mut rgba = vec![0u8; (SIZE * SIZE * 4) as usize];
    let center = (SIZE as f32 - 1.0) / 2.0;
    for y in 0..SIZE {
        for x in 0..SIZE {
            let dx = x as f32 - center;
            let dy = y as f32 - center;
            let distance = (dx * dx + dy * dy).sqrt();
            let outer = disc(distance, RADIUS);
            if outer == 0.0 {
                continue;
            }
            let (rgb, alpha) = match look {
                Look::On(color) => {
                    let fill = color_rgb(color);
                    let inner = disc(distance, RADIUS - 1.5);
                    // Blend the fill into a rim at 60 % brightness.
                    let rgb = fill.map(|c| {
                        let rim = f32::from(c) * 0.6;
                        (rim + (f32::from(c) - rim) * inner).round() as u8
                    });
                    (rgb, outer)
                }
                Look::Off | Look::NoLamp => {
                    let hole = disc(distance, RADIUS - 2.5);
                    let ring = outer * (1.0 - hole);
                    let strength = if look == Look::Off { 1.0 } else { 0.55 };
                    ([0x9a, 0xa0, 0xa6], ring * strength)
                }
            };
            let i = ((y * SIZE + x) * 4) as usize;
            rgba[i..i + 3].copy_from_slice(&rgb);
            rgba[i + 3] = (alpha * 255.0).round() as u8;
        }
    }
    Image::new_owned(rgba, SIZE, SIZE)
}

/// Creates the icon and its menu. On Linux the click events do not exist:
/// the menu gives access to the popup.
pub fn create<R: Runtime>(app: &AppHandle<R>) -> tauri::Result<()> {
    let open = MenuItem::with_id(app, "open", "Open DISLED", true, None::<&str>)?;
    let diagnostics = MenuItem::with_id(app, "diagnostics", "Diagnostics…", true, None::<&str>)?;
    let separator = PredefinedMenuItem::separator(app)?;
    let quit = MenuItem::with_id(app, "quit", "Quit DISLED", true, None::<&str>)?;
    let menu = Menu::with_items(app, &[&open, &diagnostics, &separator, &quit])?;

    TrayIconBuilder::with_id(TRAY_ID)
        .icon(render(Look::NoLamp))
        .tooltip(Look::NoLamp.tooltip())
        .menu(&menu)
        .show_menu_on_left_click(false)
        .on_menu_event(|app, event| match event.id().as_ref() {
            "open" => windows::show_popup(app),
            "diagnostics" => windows::open_diagnostics(app),
            "quit" => app.exit(0),
            _ => {}
        })
        .on_tray_icon_event(|tray, event| {
            tauri_plugin_positioner::on_tray_event(tray.app_handle(), &event);
            if let TrayIconEvent::Click {
                button: MouseButton::Left,
                button_state: MouseButtonState::Up,
                ..
            } = event
            {
                windows::toggle_popup(tray.app_handle());
            }
        })
        .build(app)?;
    Ok(())
}

/// Shows the lamp state in the icon and its tooltip.
pub fn show<R: Runtime>(app: &AppHandle<R>, look: Look) {
    if let Some(tray) = app.tray_by_id(TRAY_ID) {
        let _ = tray.set_icon(Some(render(look)));
        let _ = tray.set_tooltip(Some(look.tooltip()));
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn pixel(image: &Image<'_>, x: u32, y: u32) -> [u8; 4] {
        let i = ((y * SIZE + x) * 4) as usize;
        let rgba = image.rgba();
        [rgba[i], rgba[i + 1], rgba[i + 2], rgba[i + 3]]
    }

    #[test]
    fn a_lit_lamp_is_a_full_dot_in_its_color() {
        let image = render(Look::On(LampColor::Red));
        assert_eq!(pixel(&image, 16, 16), [0xe7, 0x4c, 0x3c, 255]);
        assert_eq!(pixel(&image, 0, 0)[3], 0, "corners stay transparent");
    }

    #[test]
    fn an_unlit_lamp_is_a_ring() {
        let image = render(Look::Off);
        assert_eq!(pixel(&image, 16, 16)[3], 0, "the middle is empty");
        assert!(pixel(&image, 16, 4)[3] > 200, "the ring is drawn");
        let faint = render(Look::NoLamp);
        assert!(pixel(&faint, 16, 4)[3] < pixel(&image, 16, 4)[3]);
    }
}
