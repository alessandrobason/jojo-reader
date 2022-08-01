# import urllib.request
import requests
from bs4 import BeautifulSoup
import sys
from os.path import exists
from os import mkdir

chap = sys.argv[1]
base = "https://steel-ball-run.com/manga/jojos-bizarre-adventure-steel-ball-run-chapter-{}/"

filepath = f"cache/chap-{chap}.txt"

if not exists("cache/"):
    mkdir("cache")

# we already loaded this file, no need to do it again
if exists(filepath):
    exit()

url = base.format(chap)

page = requests.get(url).text
soup = BeautifulSoup(page, "html.parser")

fp = open(filepath, "w", newline='\n')

for img in soup.find_all("img"):
    fp.write(img.get("src") + "\n")