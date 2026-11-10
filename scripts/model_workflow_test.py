"""Exercise the queue and Git boundaries with temporary repos and fake CLIs."""

import json
import os
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

import model_workflow as workflow


class WorkflowTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.folder = Path(temp.name).resolve()
        self.repo = self.folder / "source"
        self.repo.mkdir()
        self.git("init", "-q")
        (self.repo / "README.md").write_text("Original\n")
        self.git("add", "README.md")
        self.git("-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                 "commit", "--no-gpg-sign", "-qm", "Base")
        self.base = self.git("rev-parse", "HEAD").decode().strip()
        self.ticket = dict(id="example", base=self.base, paths=["README.md"],
                           task="Update README", acceptance="Document the example")
        self.record = self.folder / "record"
        self.record.mkdir()
        self.bin = self.folder / "bin"
        self.bin.mkdir()

    def git(self, *args):
        return workflow.git(self.repo, *args)

    def cli(self, name, body):
        executable = self.bin / name
        executable.write_text(f"#!{sys.executable}\n{body}\n")
        executable.chmod(0o700)

    def execute(self, writer, review="PASS"):
        self.cli("codex", writer)
        self.cli("agy", "import json, sys\n"
                 "assert '--sandbox' in sys.argv\n"
                 "assert 'diff --git' in sys.argv[2]\n"
                 f"print(json.dumps(dict(status='SUCCESS', response={review!r})))")
        checks = ((sys.executable, "-c", "assert open('README.md').read() == 'Updated'"),)
        with patch.object(workflow, "ROOT", self.repo), patch.object(workflow, "CHECKS", checks):
            with patch.dict(os.environ, {"PATH": f"{self.bin}:{os.environ['PATH']}"}):
                return workflow.run_ticket(self.ticket, self.record, "gemini-test")

    def test_valid_candidate_leaves_source_untouched(self):
        sha = self.execute("open('README.md', 'w').write('Updated')")
        self.assertEqual((self.record / "candidate.txt").read_text().strip(), sha)
        self.assertEqual(self.git("rev-parse", "HEAD").decode().strip(), self.base)
        self.assertFalse(self.git("status", "--porcelain"))

    def test_out_of_scope_write_stops_before_checks(self):
        with self.assertRaisesRegex(ValueError, "outside ticket paths"):
            self.execute("open('unexpected', 'w').write('outside scope')")
        self.assertFalse((self.record / "check-1.log").exists())

    def test_review_rejection_keeps_candidate_and_evidence(self):
        with self.assertRaisesRegex(ValueError, "review did not pass"):
            self.execute("open('README.md', 'w').write('Updated')", review="CHANGES")
        self.assertTrue((self.record / "candidate.txt").is_file())
        self.assertTrue((self.record / "gemini.log").is_file())

    def test_queue_disallows_traversal_and_duplicate_ids(self):
        queue = self.folder / "queue.json"
        queue.write_text(json.dumps([dict(self.ticket, paths=["docs/../README.md"])]))
        with self.assertRaisesRegex(ValueError, "exact Markdown paths"):
            workflow.load_queue(queue)
        queue.write_text(json.dumps([self.ticket, self.ticket]))
        with self.assertRaisesRegex(ValueError, "unique"):
            workflow.load_queue(queue)


if __name__ == "__main__":
    unittest.main()
