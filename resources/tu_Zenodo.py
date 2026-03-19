# Copyright (c) 2026 CyberCortex Robotics SRL. All rights reserved
# Author: Sorin Mihai Grigorescu (Python Port)

import os
import sys
import argparse
import requests
import json
from pathlib import Path


class CZenodo:
    def __init__(self, credentials_path=None, url=None, token=None):
        self.url = ""
        self.token = ""
        self.deposits = []
        self.active = False

        if credentials_path:
            self._load_credentials(credentials_path)
        elif url and token:
            self.set_auth_headers(url, token)

    def _load_credentials(self, path):
        """Simple parser for the .conf file format."""
        if not os.path.exists(path):
            print(f"Error: Credentials file '{path}' not found.")
            return

        creds = {}
        try:
            with open(path, 'r') as f:
                for line in f:
                    if '=' in line:
                        key, value = line.split('=', 1)
                        creds[key.strip()] = value.strip().strip('"').strip("'")

            url = creds.get("ZENODO_URL")
            token = creds.get("ACCESS_TOKEN")

            if url and token:
                self.set_auth_headers(url, token)
            else:
                print("Error: Missing ZENODO_URL or ACCESS_TOKEN in config.")
        except Exception as e:
            print(f"Error reading config: {e}")

    def set_auth_headers(self, url, token):
        self.url = url.rstrip('/')
        self.token = token
        headers = {"Authorization": f"Bearer {self.token}"}

        try:
            response = requests.get(self.url, headers=headers)
            if response.status_code == 200:
                data = response.json()

                # Check if it's the Records API structure (has 'hits')
                # or the Deposition API structure (is a list)
                if isinstance(data, dict) and "hits" in data:
                    items = data["hits"]["hits"]
                elif isinstance(data, list):
                    items = data
                else:
                    items = []

                # Use 'metadata' block for Records API, or top-level for Deposition API
                self.deposits = []
                for d in items:
                    # Records API wraps title/type inside 'metadata'
                    title = d.get("metadata", {}).get("title") if "metadata" in d else d.get("title")
                    self.deposits.append({"id": d["id"], "title": title})

                self.active = True
                return True
        except Exception as e:
            print(f"Connection error: {e}")

        self.active = False
        return False

    def find_deposit(self, name):
        """Search Zenodo for a record with a specific title."""
        headers = {"Authorization": f"Bearer {self.token}"}
        # Use the 'q' parameter to search specifically for the title
        # We also use 'size=1' because we only need the first match
        params = {'q': f'title:"{name}"', 'size': 1}

        try:
            r = requests.get(self.url, headers=headers, params=params)
            if r.status_code == 200:
                data = r.json()

                # Handle both Records API (hits) and Deposition API (list)
                items = data["hits"]["hits"] if isinstance(data, dict) and "hits" in data else data

                if items and len(items) > 0:
                    # Double check the title matches exactly
                    # Records API stores title in ['metadata']['title']
                    item = items[0]
                    metadata = item.get("metadata", item)
                    if metadata.get("title") == name:
                        return item["id"]
        except Exception as e:
            print(f"Search error: {e}")

        return -1

    def create_deposit(self, title, description, upload_type="dataset"):
        if not self.active: return -1

        payload = {
            "metadata": {
                "title": title,
                "upload_type": upload_type,
                "description": description,
                "creators": [{"name": "CyberCortex Robotics"}]
            }
        }
        headers = {"Content-Type": "application/json", "Authorization": f"Bearer {self.token}"}

        r = requests.post(self.url, headers=headers, data=json.dumps(payload))
        if r.status_code in [200, 201]:
            new_id = r.json().get("id")
            self.deposits.append({"id": new_id, "title": title})
            return new_id
        return -1

    def get_files(self, deposit_id):
        if not self.active: return []

        headers = {"Authorization": f"Bearer {self.token}"}
        r = requests.get(f"{self.url}/{deposit_id}/files", headers=headers)

        if r.status_code == 200:
            data = r.json()
            # Records API (Published) stores files in 'entries'
            if isinstance(data, dict) and "entries" in data:
                return data["entries"]
            # Deposition API (Drafts) returns a direct list
            elif isinstance(data, list):
                return data
        return []

    def upload_file(self, deposit_id, filepath):
        if not self.active or not os.path.exists(filepath):
            return False

        url = f"{self.url}/{deposit_id}/files"
        filename = os.path.basename(filepath)

        with open(filepath, 'rb') as f:
            files = {'file': (filename, f), 'name': (None, filename)}
            headers = {"Authorization": f"Bearer {self.token}"}
            r = requests.post(url, headers=headers, files=files)

        return r.status_code in [200, 201]

    def download_file(self, deposit_id, filename_on_zenodo, local_path):
        files = self.get_files(deposit_id)
        target_file = next((f for f in files if f['filename'] == filename_on_zenodo), None)

        if not target_file:
            return False

        download_url = target_file['links']['download']
        headers = {"Authorization": f"Bearer {self.token}"}

        with requests.get(download_url, headers=headers, stream=True) as r:
            r.raise_for_status()
            with open(local_path, 'wb') as f:
                for chunk in r.iter_content(chunk_size=8192):
                    f.write(chunk)
        return True


def main():
    parser = argparse.ArgumentParser(description="tu_Zenodo Python Equivalent", add_help=False)
    parser.add_argument("--r", help="Region name")
    parser.add_argument("--n", help="Create a new region")
    parser.add_argument("--l", action="store_true", help="List available maps")
    parser.add_argument("--u", help="Upload map path")
    parser.add_argument("--d", help="Download map name")
    parser.add_argument("--o", help="Output path for download")
    parser.add_argument("credentials", help="Path to credentials.conf")

    if len(sys.argv) < 2:
        parser.print_help()
        sys.exit(1)

    args = parser.parse_args()

    zenodo = CZenodo(credentials_path=args.credentials)
    if not zenodo.active:
        print("Could not connect to Zenodo")
        sys.exit(1)

    print("\nConnected successfully to Zenodo")

    region_name = args.n if args.n else args.r
    if not region_name:
        print("Error: Region name (--r or --n) is required.")
        sys.exit(1)

    deposit_id = zenodo.find_deposit(region_name)

    # Handle New Region Creation
    if args.n:
        if deposit_id > 0:
            print(f"Region '{region_name}' already exists with ID '{deposit_id}'.")
            sys.exit(1)
        else:
            new_id = zenodo.create_deposit(region_name, f"Region {region_name}")
            if new_id > 0:
                print(f"Region '{region_name}' created successfully with ID '{new_id}'.")
            sys.exit(0)

    # Check if existing region exists
    if deposit_id <= 0:
        print(f"Region '{region_name}' could not be found.")
        sys.exit(1)
    else:
        print(f"Region '{region_name}' found with ID '{deposit_id}'.")

    # List Maps
    if args.l:
        print(f"\nAvailable maps for '{region_name}':")
        maps = zenodo.get_files(deposit_id)
        for m in maps:
            # Use .get() to avoid KeyErrors if one doesn't exist
            fname = m.get('key') or m.get('filename') or "unknown_file"
            fid = m.get('id') or m.get('file_id') or "N/A"
            print(f"\t{fname}\t{fid}")

    # Upload
    if args.u:
        if zenodo.upload_file(deposit_id, args.u):
            print(f"\nMap '{args.u}' uploaded successfully.")

    # Download
    if args.d:
        out_path = args.o if args.o else os.getcwd()
        if os.path.isdir(out_path):
            out_path = os.path.join(out_path, args.d)

        if zenodo.download_file(deposit_id, args.d, out_path):
            print(f"\nMap file '{args.d}' successfully downloaded to '{out_path}'")
        else:
            print(f"\nCould not download map file '{args.d}'")

    print("\nEXIT_SUCCESS")


if __name__ == "__main__":
    main()
