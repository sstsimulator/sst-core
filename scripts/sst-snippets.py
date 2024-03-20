#! /usr/bin/env python3

import os
import sys
import json

if not os.path.exists("snippets"):
  os.mkdir("snippets")

srcpath = os.path.dirname(sys.argv[1])

configFile = open(sys.argv[1],'r')
config = json.load(configFile)

for tag in config["snippets"]:
  snip = config["snippets"][tag]

  # python or c++?
  language = "c++"
  try: 
    if snip["language"] == "python":
      language = "python"
  except: pass

  # a unique tag
  scoped_tag = "SSTSnippet::" + tag
  
  # read the input file
  infile = open( os.path.join(srcpath, snip["file"]), 'r')
  lines = infile.readlines() 
  
  # open the output file
  outfile = open( "snippets/" + snip["file"].replace("/","-") + '-' + tag, 'w') 

  snippeting = False
  for line in lines:
    if line.find(scoped_tag) != -1:
      if line.find("start") != -1:
        snippeting = True
      elif line.find("pause") != -1:
        snippeting = False
      elif line.find("end") != -1:
        break

      if line.find("highlight") != -1:
        if (language == "python"):
          outfile.write("#")
        else:
          outfile.write("//")
        if line.find("highlight-start") != -1:
          outfile.write("highlight-start")
        elif line.find("highlight-stop") != -1:
          outfile.write("highlight-stop")
        else:
          outfile.write("highlight-next-line\n")   
    elif snippeting and line.find("SSTSnippet::") == -1:
      outfile.write(line)

  infile.close()
  outfile.close()
      
configFile.close()
      
    
