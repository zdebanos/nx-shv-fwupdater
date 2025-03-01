# Copyright: Michal Lenc 2025 <michallenc@seznam.cz>
#            Stepan Pressl 20025 <pressl.stepan@gmail.com>

"""Implementation of PyQt6 GUI application."""

import argparse
import asyncio
import logging
import sys
from typing import Any

from PyQt6.QtCore import Qt, QThread
from PyQt6.QtWidgets import (
    QApplication,
    QComboBox,
    QDialog,
    QGridLayout,
    QLabel,
    QLineEdit,
    QProgressBar,
    QPushButton,
)

from shvconfirm import shv_confirm
from shvflasher import shv_flasher

log_levels = (
    logging.DEBUG,
    logging.INFO,
    logging.WARNING,
    logging.ERROR,
    logging.CRITICAL,
)

PROGRESS_STYLE = """
QProgressBar{
    border: 2px solid grey;
    border-radius: 5px;
    text-align: center
}

QProgressBar::chunk {
    background-color: green;
}
"""


def parse_args() -> argparse.Namespace:
    """Parse passed arguments and return result."""
    parser = argparse.ArgumentParser(
        description="GUI application for NuttX firmware flash over SHV"
    )
    parser.add_argument(
        "-v",
        action="count",
        default=0,
        help="Increase verbosity level of logging",
    )
    parser.add_argument(
        "-q",
        action="count",
        default=0,
        help="Decrease verbosity level of logging",
    )
    return parser.parse_args()


async def flashThreadAsync(url: str, img: str, path_to_root, progress_bar: QProgressBar) -> None:
    queue: asyncio.Queue = asyncio.Queue()
    task = asyncio.create_task(shv_flasher(url, img, path_to_root, queue))

    while True:
        progressVal = await queue.get()
        progress_bar.setValue(progressVal)

        if progressVal == 100:
            break

    await task


class flashThread(QThread):
    def __init__(
        self, parent: Any, url: str, img: str, path_to_root: str, progress_bar: QProgressBar
    ) -> None:
        QThread.__init__(self, parent)
        self.url = url
        self.img = img
        self.path_to_root = path_to_root
        self.progress_bar = progress_bar

    def run(self) -> None:
        asyncio.run(flashThreadAsync(self.url, self.img, self.path_to_root, self.progress_bar))


class confirmThread(QThread):
    def __init__(self, parent: Any, url: str, path_to_root: str) -> None:
        QThread.__init__(self, parent)
        self.url = url
        self.path_to_root = path_to_root

    def run(self) -> None:
        asyncio.run(shv_confirm(self.url, self.path_to_root))


class ShvFlasherGui(QDialog):
    def __init__(self) -> None:
        super().__init__(None)
        self.setWindowModality(Qt.WindowModality.ApplicationModal)
        self.setWindowTitle("NuttX Firmware Flasher over SHV")

        lab1 = QLabel("SHV RCP URL")
        self.set_interface()

        lab2 = QLabel("Image path")
        self.image = QLineEdit("update.img", self)
        self.progress_bar = QProgressBar(self)
        self.progress_bar.setValue(0)
        self.progress_bar.setStyleSheet(PROGRESS_STYLE)

        pb_flash = QPushButton("FLASH")
        pb_confirm = QPushButton("CONFIRM")
        pb_exit = QPushButton("EXIT")
        grid = QGridLayout()

        grid.addWidget(lab1, 0, 0)
        grid.addWidget(self.interface, 0, 1)
        grid.addWidget(lab2, 1, 0)
        grid.addWidget(self.image, 1, 1)
        grid.addWidget(self.progress_bar, 2, 0, 1, 3)
        grid.addWidget(pb_flash, 3, 0)
        grid.addWidget(pb_confirm, 3, 1)
        grid.addWidget(pb_exit, 3, 2)

        pb_flash.clicked.connect(self.do_flash)
        pb_confirm.clicked.connect(self.do_confirm)
        pb_exit.clicked.connect(self.reject)

        self.setLayout(grid)

    def set_interface(self) -> None:
        self.interface = QComboBox()
        self.interface.setEditable(True)
        # tcp://user@localhost:3755?password=pass
        password = "admin password"
        self.interface.addItems([
            F"tcp://admin@147.32.87.165:3755?password={password}"])

    def do_flash(self) -> None:
        rpc_url = str(self.interface.currentText())
        img_path = str(self.image.text())
        path_to_root = str("test/SaMoCon-SHV")
        workerFlash = flashThread(self, rpc_url, img_path, path_to_root, self.progress_bar)
        workerFlash.start()

    def do_confirm(self) -> None:
        rpc_url = str(self.interface.currentText())
        path_to_root = str("test/SaMoCon-SHV")
        worker = confirmThread(self, rpc_url, path_to_root)
        worker.start()


def main() -> None:
    args = parse_args()
    logging.basicConfig(
        level=log_levels[sorted([1 - args.v + args.q, 0, len(log_levels) - 1])[1]],
        format="[%(asctime)s] [%(levelname)s] - %(message)s",
    )

    _ = QApplication(sys.argv)

    dialog = ShvFlasherGui()
    dialog.exec()


if __name__ == "__main__":
    main()
