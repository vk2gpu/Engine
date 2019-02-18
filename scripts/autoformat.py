import fnmatch
import os
import subprocess

def findSource(root):
	matches = []
	for root, dirnames, filenames in os.walk(root):
		for ext in ["*.cpp", "*.h", "*.inl"]:
			for filename in fnmatch.filter(filenames, ext):
				matches.append(os.path.join(root, filename))
	return matches

def findTempFiles(root):
	matches = []
	for root, dirnames, filenames in os.walk(root):
		for ext in ["*.cpp~*.TMP", "*.h~*.TMP", "*.inl~*.TMP"]:
			for filename in fnmatch.filter(filenames, ext):
				matches.append(os.path.join(root, filename))
	return matches


def findClangFormat():
	check = "C:\\Program Files\\LLVM\\bin\\clang-format.exe"
	if os.path.exists(check):
		return check
	check = "C:\\Program Files (x86)\\LLVM\\bin\\clang-format.exe"
	if os.path.exists(check):
		return check
	check = os.environ.get("CLANG_PATH","") + "/clang-format"
	if os.path.exists(check):
		return check
	check = "clang-format"
	if os.path.exists(check):
		return check
	return None

def autoformat():
	foundSource = findSource("src")
	foundSource += findSource("apps")

	filteredSource = []
	# filter out folders with "_clang_fornat_ignore"
	for source in foundSource:
		ignoreFile = os.path.join(os.path.dirname(source), "_clang_format_ignore")
		if os.path.exists(ignoreFile) == False:
			filteredSource.append(source)

	clangFormat = findClangFormat()
	if clangFormat != None:
		for source in filteredSource:
			print " -", source
			subprocess.call([clangFormat, "-i", source])

	foundTemp = findTempFiles("src")
	foundTemp += findTempFiles("apps")

	filteredTemp = []
	# filter out folders with "_clang_fornat_ignore"
	for temp in foundTemp:
		ignoreFile = os.path.join(os.path.dirname(temp), "_clang_format_ignore")
		if os.path.exists(ignoreFile) == False:
			filteredTemp.append(temp)
	for temp in filteredTemp:
		os.remove(temp)

	pass

