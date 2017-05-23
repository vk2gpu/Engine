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


def findClangFormat():
	check = "C:\\Program Files (x86)\\LLVM\\bin\\clang-format.exe"
	if os.path.exists(check):
		return check
	return None

def autoformat():
	foundSource = findSource("src")

	filteredSource = []
	# filter out folders with "_clang_fornat_ignore"
	for source in foundSource:
		ignoreFile = os.path.join(os.path.dirname(source), "_clang_format_ignore")
		if os.path.exists(ignoreFile) == False:
			filteredSource.append(source)

	clangFormat = findClangFormat()
	if clangFormat != None:
		for source in filteredSource:
			subprocess.call([clangFormat, "-i", source])

	pass

