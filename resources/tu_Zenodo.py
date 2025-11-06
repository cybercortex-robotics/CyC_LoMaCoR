# Copyright (c) 2025 CyberCortex Robotics SRL. All rights reserved
# Author: Sorin Mihai Grigorescu

import requests

# 200: OK
# 201: Created
# 202: Accepted
# 204: No content
# 400: Bad request
# 403: Permission denied (forbidden)
# 404: Not found
# 405: Method not allowed

ACCESS_TOKEN = "LP7zVxmRWR4WTRVWNn8pMEnyBNxyTpAEui3E2pKPMZ00tpGdtzDrLZB2dgFN"
ZENODO_URL = "https://sandbox.zenodo.org/api/deposit/depositions"

def upload_file(deposition_id, filepath):
    filename = filepath.split('\\')[-1]
    data = {'name': filename}
    files = {'file': open(filepath, 'rb')}
    r_add_file = requests.post('%s/%s/files' % (ZENODO_URL, deposition_id),
                               data=data, files=files,
                               headers={'Authorization': f'Bearer {ACCESS_TOKEN}'})
    print(r_add_file.status_code)
    print(r_add_file.json())

def download_file(download_url, filename):
    r_file = requests.get(download_url, headers=headers, stream=True)
    if r_file.status_code == 200:
        with open(filename, 'wb') as f:
            for chunk in r_file.iter_content():
                f.write(chunk)
        print(f"Successfully downloaded '{filename}'.")
    else:
        print(f"Failed to download file. Status code: {r_file.status_code}")
        print(r_file.text)


# Create a new deposition
data = {
     'metadata': {
         'title': 'First upload',
         'upload_type': 'poster',
         'description': 'First map upload',
         'creators': [{'name': 'Grigorescu, Sorin',
                       'affiliation': 'CyberCortex Robotics'}]
     }
}
headers = {
    "Content-Type": "application/json",
    "Authorization": f"Bearer {ACCESS_TOKEN}"
}
# r = requests.post('https://sandbox.zenodo.org/api/deposit/depositions', json=data, headers=headers)

# Access existing depositions
r_deposits = requests.get(ZENODO_URL, headers=headers)

# Deposit ID
deposition_id = r_deposits.json()[-1]["id"]
print("deposition_id:", deposition_id)

# Read list of files
r_files = requests.get('%s/%s/files' % (ZENODO_URL, deposition_id), headers=headers)
print(r_files.status_code)
print(r_files.json())

# Upload file
#upload_file(deposition_id, 'exploratory.zip')

# Download file
download_url = r_files.json()[0]['links']['download']
filename = r_files.json()[0]['filename']
download_file(download_url, filename)
