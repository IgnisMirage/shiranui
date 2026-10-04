import json
import math
import unittest

from mqttsn_bridge.commands import (
    CommandDecoder, CommandError, Path, Pause, Resume)


def js(**kwargs):
    return json.dumps(kwargs).encode()


class DecoderTest(unittest.TestCase):
    def setUp(self):
        self.now = 0.0
        self.dec = CommandDecoder(clock=lambda: self.now)

    def test_pause_resume(self):
        self.assertEqual(self.dec.decode('pause', b'{}'), Pause())
        self.assertEqual(self.dec.decode('resume', b'{}'), Resume())

    def test_unknown_topic(self):
        with self.assertRaises(CommandError):
            self.dec.decode('other', b'{}')

    def test_token(self):
        dec = CommandDecoder(token='s3cret', clock=lambda: 0.0)
        with self.assertRaises(CommandError):
            dec.decode('pause', b'{}')
        with self.assertRaises(CommandError):
            dec.decode('pause', js(token='wrong'))
        with self.assertRaises(CommandError):
            dec.decode('pause', js(token=123))
        self.assertEqual(dec.decode('pause', js(token='s3cret')), Pause())

    def chunk(self, pid, seq, n, poses):
        return js(id=pid, seq=seq, n=n, poses=poses)

    def test_path_in_order(self):
        self.assertIsNone(self.dec.decode('path', self.chunk('a', 0, 2, [[0, 0, 0], [1, 0, 0]])))
        path = self.dec.decode('path', self.chunk('a', 1, 2, [[2, 0, 0]]))
        self.assertEqual(path, Path([(0.0, 0.0, 0.0), (1.0, 0.0, 0.0), (2.0, 0.0, 0.0)]))

    def test_path_out_of_order_and_duplicate(self):
        self.assertIsNone(self.dec.decode('path', self.chunk('a', 1, 2, [[2, 0, 0]])))
        self.assertIsNone(self.dec.decode('path', self.chunk('a', 1, 2, [[2, 0, 0]])))
        path = self.dec.decode('path', self.chunk('a', 0, 2, [[0, 0, 0], [1, 0, 0]]))
        self.assertEqual([p[0] for p in path.poses], [0.0, 1.0, 2.0])

    def test_single_chunk_path(self):
        path = self.dec.decode('path', self.chunk('a', 0, 1, [[0, 0, 0], [1, 1, 0]]))
        self.assertEqual(len(path.poses), 2)

    def test_new_id_discards_partial_path(self):
        self.dec.decode('path', self.chunk('a', 0, 2, [[9, 9, 0]]))
        self.assertIsNone(self.dec.decode('path', self.chunk('b', 0, 2, [[0, 0, 0]])))
        path = self.dec.decode('path', self.chunk('b', 1, 2, [[1, 0, 0]]))
        self.assertEqual([p[0] for p in path.poses], [0.0, 1.0])

    def test_partial_path_times_out(self):
        self.dec.decode('path', self.chunk('a', 0, 2, [[9, 9, 0]]))
        self.now = 6.0
        self.assertIsNone(self.dec.decode('path', self.chunk('a', 1, 2, [[1, 0, 0]])))

    def test_path_validation(self):
        bad = [
            self.chunk('a', 2, 2, [[0, 0, 0]]),            # seq >= n
            self.chunk('a', 0, 0, [[0, 0, 0]]),            # n = 0
            self.chunk('a', 0, 1000, [[0, 0, 0]]),         # n too large
            self.chunk('a', 0, 1, []),                      # no poses
            self.chunk('a', 0, 1, [[0, 0]]),               # not 3 values
            self.chunk('a', 0, 1, [[0, 0, 'x']]),
            self.chunk('a', 0, 1, [[0, 9999, 0]]),         # out of range
            self.chunk('', 0, 1, [[0, 0, 0]]),             # empty id
            self.chunk('a', 0, 1, [[0, 0, 0]]),            # only one pose in total
        ]
        for payload in bad:
            self.setUp()
            with self.assertRaises(CommandError, msg=payload):
                self.dec.decode('path', payload)

    def test_path_rate_limit(self):
        two = [[0, 0, 0], [1, 0, 0]]
        self.dec.decode('path', self.chunk('a', 0, 1, two))
        self.now = 0.1
        with self.assertRaises(CommandError):
            self.dec.decode('path', self.chunk('b', 0, 1, two))
        self.now = 0.3
        self.assertIsNotNone(self.dec.decode('path', self.chunk('c', 0, 1, two)))

    def test_path_length_limit(self):
        dec = CommandDecoder(max_path_poses=3, clock=lambda: 0.0)
        dec.decode('path', self.chunk('a', 0, 2, [[0, 0, 0], [1, 0, 0]]))
        with self.assertRaises(CommandError):
            dec.decode('path', self.chunk('a', 1, 2, [[2, 0, 0], [3, 0, 0]]))
        # 失敗後は新しい経路を受け付けられる
        self.assertIsNotNone(dec.decode('path', self.chunk('b', 0, 1, [[0, 0, 0], [1, 0, 0]])))


if __name__ == '__main__':
    unittest.main()
