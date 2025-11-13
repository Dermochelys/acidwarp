#!/usr/bin/env python3
"""
Rename screenshot files based on manifest.json from xcresult export.

This script processes the manifest.json file created by xcresulttool and renames
exported screenshot files to use clean, human-readable names without UUID suffixes.
"""

import json
import os
import re
import sys


def main():
    if not os.path.exists('manifest.json'):
        print("No manifest.json found - skipping screenshot renaming")
        sys.exit(0)

    with open('manifest.json', 'r') as f:
        manifest = json.load(f)

    for test_result in manifest:
        for attachment in test_result.get('attachments', []):
            exported_file = attachment.get('exportedFileName')
            suggested_name = attachment.get('suggestedHumanReadableName', '')

            if exported_file and suggested_name and os.path.exists(exported_file):
                # Extract base name from suggested name (remove _0_<uuid>.png pattern)
                # Pattern: "name_0_<uuid>.png" -> "name.png"
                match = re.match(r'^(.+?)_\d+_[A-Z0-9-]+\.png$', suggested_name)
                if match:
                    clean_name = match.group(1) + '.png'
                else:
                    # Fallback: just remove extension and add .png
                    clean_name = os.path.splitext(suggested_name)[0] + '.png'

                # Replace spaces with hyphens for filenames
                clean_name = clean_name.replace(' ', '-')

                # Only rename if different and target doesn't exist
                if clean_name != exported_file and not os.path.exists(clean_name):
                    os.rename(exported_file, clean_name)
                    print(f"Renamed {exported_file} -> {clean_name}")

    # Remove manifest.json after processing
    os.remove('manifest.json')
    print("Screenshot renaming complete")


if __name__ == '__main__':
    main()
