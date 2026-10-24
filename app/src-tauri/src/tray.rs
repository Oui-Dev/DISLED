// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 The DISLED contributors

//! The menu bar (macOS) or notification area (Windows) icon: a square in
//! the color of the lamp, a short menu, and the click that opens the popup.

use std::sync::Mutex;

use disled_protocol::LampColor;
use tauri::image::Image;
use tauri::menu::{Menu, MenuItem, PredefinedMenuItem};
use tauri::tray::{MouseButton, MouseButtonState, TrayIconBuilder, TrayIconEvent};
use tauri::{AppHandle, Manager, Runtime};

use crate::i18n::{Locale, LocaleState};
use crate::windows;

const TRAY_ID: &str = "disled";
const SIZE: u32 = 32;
/// Half the side of the square, in pixels.
const HALF: f32 = 14.0;
/// Corner radius, over a quarter of the side: smaller corners vanish once
/// the system shows this icon at 16 px.
const CORNER: f32 = 8.0;

/// What the icon shows.
#[derive(Debug, Clone, Copy, Default, PartialEq, Eq)]
pub enum Look {
    #[default]
    NoLamp,
    Off,
    On(LampColor),
}

impl Look {
    pub fn tooltip(self, locale: Locale) -> String {
        let key = match self {
            Look::NoLamp => "noLamp",
            Look::Off => "off",
            Look::On(LampColor::Green) => "green",
            Look::On(LampColor::Orange) => "orange",
            Look::On(LampColor::Red) => "red",
            Look::On(LampColor::WarmWhite) => "warmWhite",
        };
        locale.t(&format!("tray.tooltip.{key}"))
    }
}

/// The look on show, to word the tooltip again when the language changes.
#[derive(Default)]
pub struct TrayState(Mutex<Look>);

fn color_rgb(color: LampColor) -> [u8; 3] {
    match color {
        LampColor::Green => [0x2e, 0xcc, 0x71],
        LampColor::Orange => [0xf3, 0x9c, 0x12],
        LampColor::Red => [0xe7, 0x4c, 0x3c],
        LampColor::WarmWhite => [0xff, 0xe2, 0xb8],
    }
}

/// Signed distance (pixels, negative inside) from a point to the edge of a
/// square centred on the origin, of half side `half` and corner radius `r`.
fn square_distance(dx: f32, dy: f32, half: f32, r: f32) -> f32 {
    let qx = dx.abs() - (half - r);
    let qy = dy.abs() - (half - r);
    qx.max(0.0).hypot(qy.max(0.0)) + qx.max(qy).min(0.0) - r
}

/// Coverage (0..1) of a pixel by the square shrunk by `inset`, antialiased
/// over one pixel.
fn square(dx: f32, dy: f32, inset: f32) -> f32 {
    let distance = square_distance(dx, dy, HALF - inset, (CORNER - inset).max(0.0));
    (0.5 - distance).clamp(0.0, 1.0)
}

/// Renders the icon: a square in the lamp color with a darker rim (so warm
/// white shows on a light menu bar), a grey outline when the lamp is off, a
/// faint outline when no lamp is connected.
pub fn render(look: Look) -> Image<'static> {
    let mut rgba = vec![0u8; (SIZE * SIZE * 4) as usize];
    let center = (SIZE as f32 - 1.0) / 2.0;
    for y in 0..SIZE {
        for x in 0..SIZE {
            let dx = x as f32 - center;
            let dy = y as f32 - center;
            let outer = square(dx, dy, 0.0);
            if outer == 0.0 {
                continue;
            }
            let (rgb, alpha) = match look {
                Look::On(color) => {
                    let fill = color_rgb(color);
                    let inner = square(dx, dy, 1.5);
                    // Blend the fill into a rim at 60 % brightness.
                    let rgb = fill.map(|c| {
                        let rim = f32::from(c) * 0.6;
                        (rim + (f32::from(c) - rim) * inner).round() as u8
                    });
                    (rgb, outer)
                }
                Look::Off | Look::NoLamp => {
                    let hole = square(dx, dy, 2.5);
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

fn menu<R: Runtime>(app: &AppHandle<R>, locale: Locale) -> tauri::Result<Menu<R>> {
    let item =
        |id: &str| MenuItem::with_id(app, id, locale.t(&format!("tray.{id}")), true, None::<&str>);
    Menu::with_items(
        app,
        &[
            &item("open")?,
            &item("diagnostics")?,
            &PredefinedMenuItem::separator(app)?,
            &item("quit")?,
        ],
    )
}

/// Creates the icon and its menu. On Linux the click events do not exist:
/// the menu gives access to the popup.
pub fn create<R: Runtime>(app: &AppHandle<R>, locale: Locale) -> tauri::Result<()> {
    TrayIconBuilder::with_id(TRAY_ID)
        .icon(render(Look::NoLamp))
        .tooltip(Look::NoLamp.tooltip(locale))
        .menu(&menu(app, locale)?)
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
    if let Ok(mut shown) = app.state::<TrayState>().0.lock() {
        *shown = look;
    }
    if let Some(tray) = app.tray_by_id(TRAY_ID) {
        let _ = tray.set_icon(Some(render(look)));
        let _ = tray.set_tooltip(Some(look.tooltip(app.state::<LocaleState>().get())));
    }
}

/// Words the menu and the tooltip in another language.
pub fn relabel<R: Runtime>(app: &AppHandle<R>, locale: Locale) {
    let look = app
        .state::<TrayState>()
        .0
        .lock()
        .map(|look| *look)
        .unwrap_or_default();
    if let Some(tray) = app.tray_by_id(TRAY_ID) {
        if let Ok(menu) = menu(app, locale) {
            let _ = tray.set_menu(Some(menu));
        }
        let _ = tray.set_tooltip(Some(look.tooltip(locale)));
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
    fn a_lit_lamp_is_a_full_square_in_its_color() {
        let image = render(Look::On(LampColor::Red));
        assert_eq!(pixel(&image, 16, 16), [0xe7, 0x4c, 0x3c, 255]);
        assert_eq!(pixel(&image, 0, 0)[3], 0, "the margin stays transparent");
        assert_eq!(pixel(&image, 5, 5)[3], 255, "a square, not a dot");
        let corner = pixel(&image, 4, 4)[3];
        assert!(corner > 0 && corner < 255, "the corners are rounded");
    }

    #[test]
    fn an_unlit_lamp_is_an_outline() {
        let image = render(Look::Off);
        assert_eq!(pixel(&image, 16, 16)[3], 0, "the middle is empty");
        assert!(pixel(&image, 16, 3)[3] > 200, "the outline is drawn");
        let faint = render(Look::NoLamp);
        assert!(pixel(&faint, 16, 3)[3] < pixel(&image, 16, 3)[3]);
    }

    #[test]
    fn every_look_has_a_tooltip_in_every_language() {
        let looks = [
            Look::NoLamp,
            Look::Off,
            Look::On(LampColor::Green),
            Look::On(LampColor::Orange),
            Look::On(LampColor::Red),
            Look::On(LampColor::WarmWhite),
        ];
        for locale in [Locale::En, Locale::Fr] {
            for look in looks {
                assert!(
                    look.tooltip(locale).starts_with("DISLED"),
                    "{look:?} in {locale:?}"
                );
            }
        }
        assert_eq!(
            Look::On(LampColor::WarmWhite).tooltip(Locale::En),
            "DISLED: warm white"
        );
    }
}
