import sys
import scripts

commands = {
	"autoformat" : ("Run autoformatting script", scripts.autoformat.autoformat)
}

if len(sys.argv) > 1:
	command = commands[sys.argv[1]]
	print "Running", sys.argv[1], "..."
	command[1]()
else:
	print "Available commands:"
	for command in commands:
		print " - ", command, ":", commands[command][0]
