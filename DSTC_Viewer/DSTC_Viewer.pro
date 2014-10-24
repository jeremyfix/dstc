TEMPLATE = subdirs
SUBDIRS = GUI JSONParser DSTC_Statistics
GUI.depends = JSONParser
DSTC_Statistics.depends = JSONParser
