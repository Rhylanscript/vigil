import contextlib
import io
import sys
import tempfile
import unittest
from pathlib import Path

import install_app


class InstallAppTests(unittest.TestCase):
    def setUp(self):
        self.temp_dir = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp_dir.cleanup)
        self.directory = Path(self.temp_dir.name)
        self.image_path = self.directory / "storage.img"
        self.source_path = self.directory / "app.vapp"

    def create_image(self, sectors=200):
        self.image_path.write_bytes(b"\0" * (sectors * install_app.SECTOR_SIZE))

    def run_install(self, vigil_path):
        with contextlib.redirect_stdout(io.StringIO()):
            install_app.install(str(self.image_path), str(self.source_path), vigil_path)

    def test_installs_file_and_creates_nested_folders(self):
        data = bytes(range(256)) * 3
        self.create_image()
        self.source_path.write_bytes(data)

        self.run_install("apps/demo")

        with self.image_path.open("rb") as image:
            entries = install_app.read_table(image)

        apps_index = install_app.find_child(entries, install_app.ROOT, "apps")
        if apps_index is None:
            self.fail("apps folder was not installed")
        demo_index = install_app.find_child(entries, apps_index, "demo")
        if demo_index is None:
            self.fail("app was not installed in the apps folder")

        demo = entries[demo_index]
        self.assertEqual(demo["size"], len(data))
        with self.image_path.open("rb") as image:
            image.seek(demo["start_lba"] * install_app.SECTOR_SIZE)
            installed = image.read(install_app.sectors_needed(len(data)) * install_app.SECTOR_SIZE)
        self.assertEqual(installed, data.ljust(len(installed), b"\0"))

    def test_replaces_existing_file(self):
        self.create_image()
        self.source_path.write_bytes(b"old app")
        self.run_install("apps/demo")

        updated = b"updated app contents"
        self.source_path.write_bytes(updated)
        self.run_install("apps/demo")

        with self.image_path.open("rb") as image:
            entries = install_app.read_table(image)
            apps_index = install_app.find_child(entries, install_app.ROOT, "apps")
            if apps_index is None:
                self.fail("apps folder was not installed")
            demo_index = install_app.find_child(entries, apps_index, "demo")
            if demo_index is None:
                self.fail("app was not installed in the apps folder")
            image.seek(entries[demo_index]["start_lba"] * install_app.SECTOR_SIZE)
            self.assertEqual(image.read(len(updated)), updated)

        self.assertEqual(
            sum(entry["used"] and entry["name"] == "demo" for entry in entries),
            1,
        )

    def test_rejects_parent_traversal_without_changing_image(self):
        self.create_image()
        self.source_path.write_bytes(b"app")
        original_image = self.image_path.read_bytes()

        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            self.run_install("apps/../outside")

        self.assertEqual(self.image_path.read_bytes(), original_image)

    def test_fails_when_file_table_is_full(self):
        self.create_image()
        self.source_path.write_bytes(b"app")
        entries: list[install_app.Entry] = [
            {
                "name": f"dir{index}",
                "start_lba": 0,
                "size": 0,
                "used": 1,
                "is_dir": 1,
                "parent": install_app.ROOT,
            }
            for index in range(install_app.MAX_FILES)
        ]
        with self.image_path.open("r+b") as image:
            install_app.write_table(image, entries)

        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            self.run_install("apps/demo")

    def test_fails_when_disk_image_has_no_room_for_file(self):
        self.create_image(sectors=install_app.DATA_START_LBA)
        self.source_path.write_bytes(b"app")

        with contextlib.redirect_stderr(io.StringIO()), self.assertRaises(SystemExit):
            self.run_install("apps/demo")

if __name__ == "__main__":
    unittest.main()
