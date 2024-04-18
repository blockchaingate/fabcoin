#!/usr/bin/env python3
# Copyright (c) 2014-2016 The Bitcoin Core developers
# Distributed under the MIT software license, see the accompanying
# file COPYING or http://www.opensource.org/licenses/mit-license.php.
"""Test the -alertnotify and -blocknotify options."""
import os
import time

from test_framework.test_framework import FabcoinTestFramework

class NotificationsTest(FabcoinTestFramework):
    def set_test_params(self):
        self.num_nodes = 2

    def setup_network(self):
        self.alert_filename = os.path.join(self.options.tmpdir, "alert.txt")
        self.block_filename = os.path.join(self.options.tmpdir, 'blocks.txt')
        self.extra_args = [["-blockversion=2",
                            "-alertnotify=echo %%s >> %s" % self.alert_filename,
                            "-blocknotify=echo %%s >> %s" % self.block_filename],
                           ["-blockversion=211"]]
        super().setup_network()

    def run_test(self):
        self.log.info("test -blocknotify")
        block_count = 10
        blocks = self.nodes[1].generate(block_count)

        # wait at most 10 seconds for expected file size before reading the content
        wait_until(lambda: os.path.isfile(self.block_filename) and os.stat(self.block_filename).st_size >= (block_count * 65), timeout=10)

        # file content should equal the generated blocks hashes
        with open(self.block_filename, 'r') as f:
            assert_equal(sorted(blocks), sorted(f.read().splitlines()))

        # Mine another 41 up-version blocks. -alertnotify should trigger on the 51st.
        self.log.info("test -alertnotify")
        self.nodes[1].generate(41)
        self.sync_all()
        # -alertnotify should trigger on the 51'st,
        # but mine and sync another to give
        # -alertnotify time to write
        self.nodes[1].generate(1)
        self.sync_all()

        # Give fabcoind 10 seconds to write the alert notification
        timeout = 10.0
        while timeout > 0:
            if os.path.exists(self.alert_filename) and os.path.getsize(self.alert_filename):
                break
            time.sleep(0.1)
            timeout -= 0.1
        else:
            assert False, "-alertnotify did not warn of up-version blocks"

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text = f.read()

        # Mine more up-version blocks, should not get more alerts:
        self.nodes[1].generate(1)
        self.sync_all()
        self.nodes[1].generate(1)
        self.sync_all()

        with open(self.alert_filename, 'r', encoding='utf8') as f:
            alert_text2 = f.read()

        if alert_text != alert_text2:
            raise AssertionError("-alertnotify excessive warning of up-version blocks")

if __name__ == '__main__':
    NotificationsTest().main()
