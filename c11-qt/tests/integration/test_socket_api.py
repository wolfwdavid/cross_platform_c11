#!/usr/bin/env python3
"""
Integration tests for c11 socket API.
Ported from tests_v2/ golden-master tests.

Usage:
    # Start c11 app first, then:
    C11_SOCKET=/path/to/c11.sock python3 test_socket_api.py
"""

import json
import os
import socket
import sys
import unittest


def get_socket_path():
    path = os.environ.get("C11_SOCKET") or os.environ.get("CMUX_SOCKET")
    if not path:
        if sys.platform == "darwin":
            path = os.path.expanduser("~/Library/Application Support/c11/c11.sock")
        else:
            runtime = os.environ.get("XDG_RUNTIME_DIR", "/tmp")
            path = os.path.join(runtime, "c11.sock")
    return path


def send_v1(command: str) -> str:
    sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    sock.connect(get_socket_path())
    sock.sendall((command + "\n").encode())
    sock.settimeout(5.0)
    data = b""
    while True:
        chunk = sock.recv(4096)
        if not chunk:
            break
        data += chunk
        if data.endswith(b"\n"):
            break
    sock.close()
    return data.decode().strip()


def send_v2(method: str, params: dict = None) -> dict:
    req = {"id": 1, "method": method}
    if params:
        req["params"] = params
    raw = json.dumps(req)
    response = send_v1(raw)
    return json.loads(response)


class TestSystemCommands(unittest.TestCase):
    def test_ping(self):
        result = send_v2("system.ping")
        self.assertTrue(result["ok"])
        self.assertEqual(result["result"], "pong")

    def test_capabilities(self):
        result = send_v2("system.capabilities")
        self.assertTrue(result["ok"])
        caps = result["result"]
        self.assertTrue(caps["v1"])
        self.assertTrue(caps["v2"])
        self.assertIn("version", caps)

    def test_tree(self):
        result = send_v2("system.tree")
        self.assertTrue(result["ok"])
        tree = result["result"]
        self.assertIn("workspaces", tree)
        self.assertGreater(len(tree["workspaces"]), 0)


class TestWorkspaceCommands(unittest.TestCase):
    def test_list_workspaces(self):
        result = send_v2("workspace.list")
        self.assertTrue(result["ok"])
        self.assertIsInstance(result["result"], list)

    def test_current_workspace(self):
        result = send_v2("workspace.current")
        self.assertTrue(result["ok"])
        ws = result["result"]
        self.assertIn("id", ws)
        self.assertIn("title", ws)

    def test_create_and_close_workspace(self):
        # Create
        result = send_v2("workspace.create", {"title": "Integration Test"})
        self.assertTrue(result["ok"])
        ws_id = result["result"]["id"]
        self.assertEqual(result["result"]["title"], "Integration Test")

        # Verify it exists
        result = send_v2("workspace.list")
        ids = [ws["id"] for ws in result["result"]]
        self.assertIn(ws_id, ids)

        # Close it
        result = send_v2("workspace.close", {"id": ws_id})
        self.assertTrue(result["ok"])

    def test_workspace_navigation(self):
        # Create a second workspace
        send_v2("workspace.create", {"title": "Nav Test"})

        result = send_v2("workspace.next")
        self.assertTrue(result["ok"])

        result = send_v2("workspace.previous")
        self.assertTrue(result["ok"])

        # Clean up
        result = send_v2("workspace.list")
        for ws in result["result"]:
            if ws["title"] == "Nav Test":
                send_v2("workspace.close", {"id": ws["id"]})


class TestSurfaceCommands(unittest.TestCase):
    def test_list_surfaces(self):
        result = send_v2("surface.list")
        self.assertTrue(result["ok"])
        self.assertIsInstance(result["result"], list)
        self.assertGreater(len(result["result"]), 0)

    def test_surface_has_type(self):
        result = send_v2("surface.list")
        for surface in result["result"]:
            self.assertIn("type", surface)
            self.assertIn(surface["type"], ["terminal", "browser", "markdown"])


class TestV1Commands(unittest.TestCase):
    def test_v1_ping(self):
        result = send_v1("ping")
        self.assertEqual(result, "pong")

    def test_v1_help(self):
        result = send_v1("help")
        self.assertIn("ping", result)
        self.assertIn("list_workspaces", result)

    def test_v1_unknown_command(self):
        result = send_v1("nonexistent_command")
        self.assertIn("ERROR", result)


if __name__ == "__main__":
    if not os.path.exists(get_socket_path()):
        print(f"ERROR: c11 socket not found at {get_socket_path()}")
        print("Start c11 first, or set C11_SOCKET environment variable.")
        sys.exit(1)

    unittest.main(verbosity=2)
