import tkinter as tk
from tkinter import filedialog


def impl_ask_open_file(title, file_types, initial_dir):
    root = tk.Tk()
    root.withdraw()

    file_path = filedialog.askopenfilename(
        title=title, initialdir=initial_dir, filetypes=file_types
    )
    # I am not sure if askopenfilename returns None or ''
    return file_path if file_path else ''


def impl_ask_save_file(title, file_types, initial_dir, def_ext):
    root = tk.Tk()
    root.withdraw()

    file_path = filedialog.asksaveasfilename(
        title=title, initialdir=initial_dir, filetypes=file_types, defaultextension=def_ext
    )
    return file_path if file_path else ''
