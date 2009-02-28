import feedparser
import time
import datetime
import tempfile
import linecache

# temporary file used to store rss entries
filename = "/tmp/rsstimer.tmp"
# interval in seconds between rss updates
updateinterval = 60

# lcd4linux permits only 1 parameter passed to the function
# we send the rss title id with the ! spacer
def getfeed(rssfeed):
    print rssfeed
    idx = 0
    feed = rssfeed.split('!')[0]
    idx = int(rssfeed.split('!')[-1])
    if (idx <= 0): idx = 1;
    
    oldfeed = fgetfeed()
    
    if (oldfeed != feed):
	lastupdate = 0
	print "Feed changed refresing"
    else:
        lastupdate = fgetseconds()
    
    if (lastupdate <= 0):
	saverss(feed)
    else:
	now = getseconds()
	delta = now - lastupdate
	if (delta > updateinterval):
	    print "Last update: " + str(delta) + " seconds ago. Updating the rss entries."
	    saverss(feed)
    # first line in the file is the timestamp second is the feed url
    output = linecache.getline(filename, idx + 2)
    print output
    return output
	
def getseconds():
    ts = datetime.datetime.now()
    return time.mktime(ts.timetuple())

def fgetseconds():
    try:
	f = open(filename, "r")
    except IOError:
	print "Cannot get timestamp from file"
	return 0
    else:
	return float(f.readline())

def fgetfeed():
    try:
	f = open(filename, "r")
    except IOError:
	print "Cannot get feed from file"
	return ' '
    else:
	# skip first line 
	f.readline()
	return f.readline().rstrip("\n")

	
def saverss(rssfeed):
    linecache.clearcache()
    f = open(filename, "w")
    # save timestamp
    f.write(str(getseconds()))
    f.write("\n")
    # save feed url
    f.write(rssfeed)
    f.write("\n")
    print "Downloading the rss feed from: " + rssfeed
    feed = feedparser.parse(rssfeed)
    for entry in feed.entries:
	f.write(entry.title)
	f.write("\n")
    f.close
    print "Done"

def printrss():
    f = open(filename, "r")
    f.readline()
    for line in f:
	print line
	
#print getfeed("http://slashdot.org/slashdot.rdf!5")
print getfeed("http://www.linux.com/feed?theme=rss!1")

