"""
Verify F5 copy behaviour: file and directory copy to the other panel,
cancel leaves nothing behind, and selecting '..' is a no-op.
"""
import os
import shutil
import sys
import tempfile
from driver import TestCase, TestDriver


class CopyTests(TestCase):

    # --- helpers ---

    def _make_src_dst(self):
        """Return (src_dir, dst_dir) as freshly created temp directories."""
        src = tempfile.mkdtemp()
        dst = tempfile.mkdtemp()
        return src, dst

    def _position(self, src, dst):
        """Place left panel at src, right panel at dst.
        Left panel focus and index 0 (..) are assumed after setpath."""
        self.app.setpath("left", src)
        self.app.setpath("right", dst)

    # --- tests ---

    def test_copy_file(self):
        src, dst = self._make_src_dst()
        try:
            open(os.path.join(src, "hello.txt"), "w").close()
            self._position(src, dst)
            # Select hello.txt (index 1, after ..)
            self.app.send("keydown down")
            self.app.dialog("yes")
            self.app.send("keydown f5")
            s = self.app.state()
            assert s["leftPath"]  == src, f"leftPath changed: {s['leftPath']}"
            assert s["rightPath"] == dst, f"rightPath changed: {s['rightPath']}"
            assert os.path.isfile(os.path.join(dst, "hello.txt")), \
                "hello.txt not found in destination"
        finally:
            shutil.rmtree(src, ignore_errors=True)
            shutil.rmtree(dst, ignore_errors=True)

    def test_copy_directory(self):
        src, dst = self._make_src_dst()
        try:
            subdir = os.path.join(src, "mydir")
            os.makedirs(subdir)
            open(os.path.join(subdir, "file.txt"), "w").close()
            self._position(src, dst)
            # Select mydir/ (index 1, after ..)
            self.app.send("keydown down")
            self.app.dialog("yes")
            self.app.send("keydown f5")
            self.app.state()  # synchronise: wait until F5 is fully processed
            assert os.path.isfile(os.path.join(dst, "mydir", "file.txt")), \
                "mydir/file.txt not found in destination after recursive copy"
        finally:
            shutil.rmtree(src, ignore_errors=True)
            shutil.rmtree(dst, ignore_errors=True)

    def test_copy_cancel(self):
        src, dst = self._make_src_dst()
        try:
            open(os.path.join(src, "hello.txt"), "w").close()
            self._position(src, dst)
            self.app.send("keydown down")
            self.app.dialog("no")
            self.app.send("keydown f5")
            assert not os.path.exists(os.path.join(dst, "hello.txt")), \
                "file was copied despite cancellation"
        finally:
            shutil.rmtree(src, ignore_errors=True)
            shutil.rmtree(dst, ignore_errors=True)

    def test_copy_dotdot_is_noop(self):
        src, dst = self._make_src_dst()
        try:
            self._position(src, dst)
            # Selected entry is '..' (index 0) — F5 must be a no-op
            before = self.app.state()
            self.app.send("keydown f5")
            after = self.app.state()
            assert after["leftSelected"] == before["leftSelected"], \
                "selection changed after F5 on .."
            assert not any(f != ".." for f in os.listdir(dst)), \
                "destination is not empty after F5 on .."
        finally:
            shutil.rmtree(src, ignore_errors=True)
            shutil.rmtree(dst, ignore_errors=True)

    def test_copy_refreshes_destination_panel(self):
        """After a successful copy the destination panel entries update."""
        src, dst = self._make_src_dst()
        try:
            open(os.path.join(src, "new.txt"), "w").close()
            self._position(src, dst)
            self.app.send("keydown down")
            self.app.dialog("yes")
            self.app.send("keydown f5")
            s = self.app.state()
            # rightEntries is serialised from the live filesystem via stateSnapshot
            assert "new.txt" in s["rightEntries"], \
                f"new.txt not in rightEntries after copy: {s['rightEntries']}"
        finally:
            shutil.rmtree(src, ignore_errors=True)
            shutil.rmtree(dst, ignore_errors=True)


if __name__ == "__main__":
    import os as _os
    _TESTS_DIR = _os.path.dirname(_os.path.abspath(__file__))
    app = TestDriver(sys.argv[1], _TESTS_DIR)
    ok = CopyTests(sys.argv[1]).run(app)
    app.quit()
    sys.exit(0 if ok else 1)
