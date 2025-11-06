#!/usr/bin/env python3
"""
Master test runner for all Gamba client layers.
Runs all test suites and reports results.
"""

import sys
import subprocess
import os


def run_test(script_name: str, description: str) -> bool:
    """
    Run a test script and return success status.
    
    Args:
        script_name: Name of test script
        description: Human-readable description
        
    Returns:
        True if test passed
    """
    print("\n" + "=" * 80)
    print(f"Running: {description}")
    print("=" * 80)
    
    try:
        result = subprocess.run(
            [sys.executable, script_name],
            capture_output=False,
            text=True,
            cwd=os.path.dirname(os.path.abspath(__file__))
        )
        
        success = result.returncode == 0
        
        if success:
            print(f"\n✓ {description} - PASSED")
        else:
            print(f"\n✗ {description} - FAILED")
        
        return success
        
    except Exception as e:
        print(f"\n✗ {description} - ERROR: {e}")
        return False


def main():
    """Run all tests"""
    print("\n" + "=" * 80)
    print("GAMBA CLIENT - COMPLETE TEST SUITE")
    print("=" * 80)
    print("\nThis will run all layer tests to verify the implementation.")
    print("Tests will check: message layer, utils layer, and game layer.")
    print("\nPress Enter to start, or Ctrl+C to cancel...")
    
    try:
        input()
    except KeyboardInterrupt:
        print("\n\nCancelled.")
        return 1
    
    tests = [
        ("test_message_layer.py", "Message Layer Tests"),
        ("test_utils_layer.py", "Utils Layer Tests"),
        ("test_game_layer.py", "Game Layer Tests"),
    ]
    
    results = []
    
    for script, description in tests:
        if not os.path.exists(script):
            print(f"\n⚠️  Warning: {script} not found, skipping...")
            results.append((description, None))
            continue
        
        success = run_test(script, description)
        results.append((description, success))
    
    # Summary
    print("\n\n" + "=" * 80)
    print("TEST SUITE SUMMARY")
    print("=" * 80)
    
    for description, result in results:
        if result is None:
            status = "⚠️  SKIPPED"
        elif result:
            status = "✓ PASSED"
        else:
            status = "✗ FAILED"
        print(f"{status:12} - {description}")
    
    passed = sum(1 for _, r in results if r is True)
    failed = sum(1 for _, r in results if r is False)
    skipped = sum(1 for _, r in results if r is None)
    total = len(results)
    
    print(f"\nResults: {passed} passed, {failed} failed, {skipped} skipped (total: {total})")
    print("=" * 80)
    
    if failed == 0 and passed > 0:
        print("\n🎉 ALL TESTS PASSED! The implementation is solid.")
        print("\nYou can now proceed to the network layer with confidence!")
        return 0
    elif failed > 0:
        print("\n⚠️  Some tests failed. Please review the errors above.")
        return 1
    else:
        print("\n⚠️  No tests were run.")
        return 1


if __name__ == "__main__":
    sys.exit(main())
