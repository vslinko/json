import os
import subprocess

FNULL = open(os.devnull, 'w')

for root, dirs, files in os.walk("./vendor/JSONTestSuit/test_parsing"):
  json_files = (f for f in files if f.endswith(".json"))
  for filename in json_files:
    file_path = os.path.join(root, filename)

    status = -1
    result = None

    try:
      status = subprocess.call(
          ["./json", file_path],
          stdin=FNULL,
          stdout=FNULL,
          stderr=subprocess.STDOUT,
          timeout=5
      )
    except subprocess.TimeoutExpired:
      result = "timeout"

    if not result:
      if status == 0:
        result = "pass"
      elif status == 1:
        result = "fail"
      else:
        result = "crash"

    s = None
    if result == "timeout":
      s = "\033[0;31mTIMEOUT\033[0m               %s" % (filename)
    if result == "crash":
      s = "\033[0;31mCRASH\033[0m                 %s" % (filename)
    elif filename.startswith("y_") and result != "pass":
      s = "\033[0;31mSHOULD_HAVE_PASSED\033[0m    %s" % (filename)
    elif filename.startswith("n_") and result == "pass":
      s = "\033[0;31mSHOULD_HAVE_FAILED\033[0m    %s" % (filename)
    elif filename.startswith("i_") and result == "pass":
      s = "\033[0;34mIMPLEMENTATION_PASS\033[0m   %s" % (filename)
    elif filename.startswith("i_") and result != "pass":
      s = "\033[0;34mIMPLEMENTATION_FAIL\033[0m   %s" % (filename)
    else:
      s = "\033[0;32mEXPECTED_RESULT\033[0m       %s" % (filename)

    print(s)
