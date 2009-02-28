import urllib
import shutil

download_path = "/tmp/"

def saveimage(imageurl):
	filename = imageurl.split('/')[-1]
	tmpname = filename + ".tmp"
	try:
		urllib.urlretrieve(imageurl, download_path + tmpname)
	except IOError:
		return "Error downloading file"
	else:
		shutil.move(download_path + tmpname, download_path + filename)
		return download_path + filename


#saveimage("http://www.switch.ch/network/operation/statistics/geant2-day.png")
#saveimage("http://192.168.12.113/mrtg/127.0.0.1_2-day.png")


