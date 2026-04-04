import subprocess
import sys


class TestDriver:
    def __init__(self, executable):
        self.proc = subprocess.Popen(
            [executable, "--test"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            text=True,
            bufsize=1,
        )
        line = self.proc.stdout.readline().strip()
        if line != "ready":
            self.proc.kill()
            raise RuntimeError(f"Expected 'ready', got: {line!r}")

    def send(self, command):
        self.proc.stdin.write(command + "\n")
        self.proc.stdin.flush()

    def quit(self):
        self.send("quit")
        self.proc.wait(timeout=5)


if __name__ == "__main__":
    exe = sys.argv[1] if len(sys.argv) > 1 else "unicommander"
    driver = TestDriver(exe)
    print("Handshake OK")
    driver.quit()
    print("Done.")
