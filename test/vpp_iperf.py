#!/usr/bin/env python

# Start an iPerf connection stream between two Linux namespaces ##

import subprocess
import os
import sys
import time
import signal


class VppIperf:
    """ "Create an iPerf connection stream between two namespaces.

    Usage:
    iperf = VppIperf()                   # Create the iPerf Object
    iperf.client_ns = 'ns1'              # Client Namespace
    iperf.server_ns = 'ns2'              # Server Namespace
    iperf.server_ip = '10.0.0.102'       # Server IP Address
    iperf.start()                        # Start the connection stream

    Optional:
    iperf.duration = 15   # Time to transmit for in seconds (Default=10)

    ## Optionally set any iperf client & server args
    Example:
    # Run 4 parallel streams, write to logfile & bind to port 5202
    iperf.client_args='-P 4 --logfile /tmp/vpp-vm-tests/vpp_iperf.log -p 5202'
    iperf.server_args='-p 5202'
    """

    def __init__(self, server_ns=None, client_ns=None, server_ip=None, logger=None):
        self.server_ns = server_ns
        self.client_ns = client_ns
        self.server_ip = server_ip
        self.duration = 10
        self.client_args = ""
        self.server_args = ""
        self.logger = logger
        # Set the iperf executable
        self.iperf = self.get_iperf()

    def ensure_init(self):
        if self.server_ns and self.client_ns and self.server_ip:
            return True
        else:
            raise Exception(
                "Error: Cannot Start." "iPerf object has not been initialized"
            )

    def get_iperf(self):
        """Return the iperf executable for running tests.

        Look for the iperf executable in the following order
        1. ${TEST_DATA_DIR}/usr/bin/iperf  # running tests inside the VM
        2. /usr/bin/iperf3                 # running tests on the host
        """
        vm_test_dir = os.getenv("TEST_DATA_DIR", "/tmp/vpp-vm-tests")
        if os.path.isdir(vm_test_dir):
            iperf = os.path.join(vm_test_dir, "/usr/bin/iperf")
        else:
            iperf = "/usr/bin/iperf3"
        if os.path.exists(iperf):
            return iperf
        else:
            self.logger.error(f"Could not find an iperf executable for running tests")
            sys.exit(1)

    def start_iperf_server(self):
        """Starts the  iperf server and returns the process cmdline args."""
        args = [
            "ip",
            "netns",
            "exec",
            self.server_ns,
            self.iperf,
            "-s",
            "-D",
        ]
        args.extend(self.server_args.split())
        cmd = " ".join(args)
        self.logger.debug(f"Starting iperf server: {cmd}")
        try:
            subprocess.run(
                cmd,
                timeout=self.duration + 5,
                encoding="utf-8",
                shell=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
        except subprocess.TimeoutExpired as e:
            raise Exception("Error: Timeout expired for iPerf", e.output)
        return args[4:]

    def start_iperf_client(self):
        args = [
            "ip",
            "netns",
            "exec",
            self.client_ns,
            self.iperf,
            "-c",
            self.server_ip,
            "-t",
            str(self.duration),
        ]
        args.extend(self.client_args.split())
        args = " ".join(args)
        try:
            return subprocess.run(
                args,
                timeout=self.duration + 5,
                encoding="utf-8",
                capture_output=True,
                shell=True,
            )
        except subprocess.TimeoutExpired as e:
            raise Exception("Error: Timeout expired for iPerf", e.output)

    def start(self, server_only=False, client_only=False):
        """Runs iPerf.

        Starts the iperf server daemon & runs the iperf client.
        arguments:-
        server_only -- start the iperf server daemon only
        client_only -- run the iperf client only
        Return True if we have no errors in iPerf client, else False.
        """
        self.ensure_init()
        if not client_only:
            return self.start_iperf_server()
        if not server_only:
            result = self.start_iperf_client()
            self.logger.debug(f"Iperf client args: {result.args}")
            self.logger.debug(result.stdout)
            if result.stderr:
                self.logger.error(
                    f"Error starting Iperf Client in Namespace: {self.client_ns}"
                )
                self.logger.error(f"Iperf client args: {result.args}")
                self.logger.error(f"Iperf client has errors: {result.stderr}")
                return False
            else:
                return True


## Functions to start and stop iPerf using the iPerf object
def start_iperf(
    ip_version,
    client_ns="iprf_client_ns",
    server_ns="iprf_server_ns",
    server_ipv4_address="10.0.0.102",
    server_ipv6_address="2001:1::2",
    client_args="",
    server_args="",
    duration=10,
    server_only=False,
    client_only=False,
    logger=None,
):
    """Start an iperf connection stream using the iPerf object.

    Starts iPerf an connection stream between an iPerf client in the
    client namespace (client_ns) and a server in another
    namespace (server_ns).
    Parameters:
    ip_version - 4 or 6
    client_ns - iPerf client namespace
    server_ns - iPerf server namespace
    server_ipv4_address - ipv4 address of the server, if ip_version=4
    server_ipv6_address - ipv6 address of the server, if ip_version=6
    client_args - Additonal iperf control arguments to be passed
                    to the iperf client from the test (str)
    server_args - Additonal iperf control arguments to be passed
                    to the iperf server from the test (str)
    duration    - Iperf duration in seconds
    server_only - start iperf server only
    client_only - start the iperf client only
    logger - test logger
    """
    if ip_version == 4:
        iperf_server_ip = server_ipv4_address
    elif ip_version == 6:
        iperf_server_ip = server_ipv6_address
        client_args = "-V" + " " + client_args
        server_args = "-V" + " " + server_args
    iperf = VppIperf()
    iperf.client_ns = client_ns
    iperf.server_ns = server_ns
    iperf.server_ip = iperf_server_ip
    iperf.client_args = client_args
    iperf.server_args = server_args
    iperf.duration = duration
    iperf.logger = logger
    return iperf.start(server_only=server_only, client_only=client_only)


def stop_iperf(iperf_cmd):
    """Stop the iperf process matching the iperf_cmd string."""
    try:
        result = subprocess.run(
            ["pgrep", "-x", "-f", iperf_cmd],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            encoding="utf-8",
        )
        pids = result.stdout.strip().split()
        if not pids:
            # No matching iperf3 processes found
            return

        for pid in pids:
            try:
                # First send SIGTERM to cleanup and notify the parent process
                os.kill(int(pid), signal.SIGTERM)
                time.sleep(2)
                os.kill(int(pid), 0)  # Check if still alive
                os.kill(int(pid), signal.SIGKILL)
            except ProcessLookupError:
                pass  # Process already exited
            except Exception as e:
                print(f"Error terminating iperf3 process {pid}: {e}")
    except Exception as e:
        print(f"Failed to run pgrep for '{iperf_cmd}': {e}")


if __name__ == "__main__":
    # Run iPerf using default settings
    iperf = VppIperf()
    iperf.client_ns = "ns1"
    iperf.server_ns = "ns2"
    iperf.server_ip = "10.0.0.102"
    iperf.duration = 20
    iperf.start()
