import importlib.util
import pathlib
import unittest
p = pathlib.Path(__file__).resolve().parents[1] / "scripts/analyze_io.py"
spec = importlib.util.spec_from_file_location("analyze_io", p)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)

class CorrelationTest(unittest.TestCase):
    def test_union_thread_boundaries_and_latest_session(self):
        text = """KH2AssetLoadDiag d3; frequency=1000 origin=0
DETAIL phase=register thread=1 start_qpc=1 end_qpc=50 cpu_valid=0 kernel_100ns=0 user_100ns=0 file="old"
KH2AssetLoadDiag d4; frequency=1000 origin=100
DETAIL phase=original_load thread=5 start_qpc=100 end_qpc=200 cpu_valid=1 kernel_100ns=100000 user_100ns=200000 file="map"
IOSTATUS installed_slots=2 live_slots=2 open_windows=0 dropped=0 clock_errors=0
IOEVENT module=game api=ReadFile thread=5 start_qpc=110 end_qpc=150
IOEVENT module=game api=WaitForSingleObject thread=5 start_qpc=120 end_qpc=160
IOEVENT module=game api=ReadFile thread=6 start_qpc=100 end_qpc=200
IOEVENT module=game api=ReadFile thread=5 start_qpc=95 end_qpc=105
"""
        result = module.analyze(text)
        self.assertEqual(len(result["calls"]), 1)
        call = result["calls"][0]
        self.assertEqual(call["observed_api_union_ms"], 50)
        self.assertEqual(call["cpu_ms"], 30)
        self.assertEqual(call["crossing_events_excluded"], 1)
        self.assertEqual(sum(v["events"] for v in call["apis"].values()), 2)

if __name__ == "__main__":
    unittest.main()
