#!/usr/bin/env python3
"""
Desktop UI for document-to-Braille conversion.

Implements an Open/Save/Edit/Convert flow inspired by the native driver UI and
connects to the existing extraction and Braille conversion system.
"""

from __future__ import annotations

import tkinter as tk
from pathlib import Path
from tkinter import filedialog, messagebox
from tkinter.scrolledtext import ScrolledText

from braille import NUMBER_INDICATOR_PATTERN, char_to_braille, pattern_to_grid
from doc_to_braille import SUPPORTED_EXTENSIONS, clean_extracted_text, extract_text
from shorthand_braille import to_shorthand_braille_report


def to_braille_report(text: str) -> str:
    """Convert plain text to the same verbose Braille report format as CLI."""
    cleaned = "".join(c for c in text if c == "\n" or (32 <= ord(c) <= 126))
    if not cleaned.strip():
        return "(No printable text in input.)"

    lines: list[str] = []
    lines.append("========================================")
    lines.append("CONVERTING TO BRAILLE")
    lines.append("========================================")
    preview_source = cleaned.replace("\n", " ").strip()
    preview = preview_source[:200] + ("..." if len(preview_source) > 200 else "")
    lines.append("Input (first 200 chars):")
    lines.append(f"\"{preview}\"")
    lines.append("----------------------------------------")
    lines.append("")

    in_number_mode = False
    char_index = 0
    total_chars = sum(1 for c in cleaned if c != "\n")

    for c in cleaned:
        if c == "\n":
            lines.append("[Newline]")
            lines.append("")
            continue

        is_digit = c.isdigit()
        if is_digit and not in_number_mode:
            lines.append("[Number Indicator]")
            lines.extend(pattern_to_grid(NUMBER_INDICATOR_PATTERN))
            lines.append("")
            in_number_mode = True
        elif not is_digit and in_number_mode:
            in_number_mode = False

        char_index += 1
        if c == " ":
            label = "'SPACE'"
        elif 33 <= ord(c) <= 126:
            label = f"'{c}'"
        else:
            label = f"(0x{ord(c):02X})"

        lines.append(f"Character {char_index}/{total_chars}: {label}")
        lines.extend(pattern_to_grid(char_to_braille(c)))
        lines.append("")

    lines.append("========================================")
    lines.append("CONVERSION COMPLETE!")
    lines.append("========================================")
    return "\n".join(lines)


class BrailleUI(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Text Sender v1.1.0 - Braille Converter")
        self.geometry("980x680")
        self.minsize(900, 600)

        self.current_file: Path | None = None
        self._build_ui()

    def _build_ui(self) -> None:
        top = tk.Frame(self, padx=10, pady=10)
        top.pack(fill=tk.X)

        self.path_var = tk.StringVar(value="No file loaded")
        path_entry = tk.Entry(top, textvariable=self.path_var, state="readonly")
        path_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)

        btn_open = tk.Button(top, text="Open", width=10, command=self.open_document)
        btn_open.pack(side=tk.LEFT, padx=(8, 4))

        btn_save = tk.Button(top, text="Save", width=10, command=self.save_text)
        btn_save.pack(side=tk.LEFT, padx=4)

        btn_convert = tk.Button(top, text="Send", width=10, command=self.convert_to_braille)
        btn_convert.pack(side=tk.LEFT, padx=(4, 0))

        mode_bar = tk.Frame(self, padx=10, pady=0)
        mode_bar.pack(fill=tk.X)
        tk.Label(mode_bar, text="Conversion Mode:").pack(side=tk.LEFT)
        self.mode_var = tk.StringVar(value="regular")
        tk.Radiobutton(
            mode_bar,
            text="Regular",
            variable=self.mode_var,
            value="regular",
        ).pack(side=tk.LEFT, padx=(8, 0))
        tk.Radiobutton(
            mode_bar,
            text="Shorthand",
            variable=self.mode_var,
            value="shorthand",
        ).pack(side=tk.LEFT, padx=(8, 0))

        center = tk.Frame(self, padx=10, pady=10)
        center.pack(fill=tk.BOTH, expand=True)

        left_panel = tk.Frame(center)
        left_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 6))

        right_panel = tk.Frame(center)
        right_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(6, 0))

        tk.Label(left_panel, text="Extracted / Editable Text").pack(anchor="w")
        self.input_box = ScrolledText(left_panel, wrap=tk.WORD)
        self.input_box.pack(fill=tk.BOTH, expand=True)

        tk.Label(right_panel, text="Braille Output").pack(anchor="w")
        self.output_box = ScrolledText(right_panel, wrap=tk.NONE)
        self.output_box.pack(fill=tk.BOTH, expand=True)
        self.output_box.configure(state=tk.DISABLED)

        bottom = tk.Frame(self, padx=10, pady=10)
        bottom.pack(fill=tk.X)
        tk.Label(
            bottom,
            text="Supported: " + ", ".join(sorted(SUPPORTED_EXTENSIONS)),
            anchor="w",
        ).pack(fill=tk.X)

    def open_document(self) -> None:
        filetypes = [
            (
                "Supported docs",
                " ".join(f"*{ext}" for ext in sorted(SUPPORTED_EXTENSIONS)),
            ),
            ("All files", "*.*"),
        ]
        selected = filedialog.askopenfilename(
            title="Open document",
            filetypes=filetypes,
        )
        if not selected:
            return

        path = Path(selected)
        try:
            raw = extract_text(path)
            cleaned = clean_extracted_text(raw)
        except SystemExit:
            messagebox.showerror("Open failed", "Missing dependency for this file type.")
            return
        except Exception as exc:
            messagebox.showerror("Open failed", f"{exc}")
            return

        self.current_file = path
        self.path_var.set(str(path))
        self.input_box.delete("1.0", tk.END)
        self.input_box.insert("1.0", cleaned)

    def save_text(self) -> None:
        output_path = filedialog.asksaveasfilename(
            title="Save text",
            defaultextension=".txt",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
        )
        if not output_path:
            return

        text = self.input_box.get("1.0", tk.END).rstrip("\n")
        try:
            Path(output_path).write_text(text, encoding="utf-8")
        except Exception as exc:
            messagebox.showerror("Save failed", f"{exc}")
            return
        messagebox.showinfo("Saved", f"Saved to {output_path}")

    def convert_to_braille(self) -> None:
        text = self.input_box.get("1.0", tk.END).rstrip("\n")
        if self.mode_var.get() == "shorthand":
            report = to_shorthand_braille_report(text)
        else:
            report = to_braille_report(text)
        self.output_box.configure(state=tk.NORMAL)
        self.output_box.delete("1.0", tk.END)
        self.output_box.insert("1.0", report)
        self.output_box.configure(state=tk.DISABLED)


def main() -> None:
    app = BrailleUI()
    app.mainloop()


if __name__ == "__main__":
    main()
