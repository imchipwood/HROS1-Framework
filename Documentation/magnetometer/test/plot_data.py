import os
import argparse
import matplotlib.pyplot as plot
import csv


def parseArgs():
	argparser = argparse.ArgumentParser()
	argparser.add_argument('-csvfile', '-c', type=str, help='path to CSV file', default='dump.csv')
	return argparser.parse_args()


def main():
	parsedArgs = parseArgs()
	csvPath = parsedArgs.csvfile

	if not os.path.exists(csvPath):
		print("Couldn't find {}, trying current dir".format(csvPath))
		csvPath = os.path.join(os.path.dirname(__file__), os.path.basename(csvPath))
		if not os.path.exists(csvPath):
			raise OSError("Couldn't find {}".format(csvPath))

	# read the CSV
	x = []
	y = []
	z = []
	with open(csvPath, 'r') as inf:
		print("opened {}".format(csvPath))
		reader = csv.DictReader(inf, fieldnames=['x', 'y', 'z'])

		for line in reader:
			# skip first line
			if line['x'] == 'x':
				continue

			# get raw values
			x.append(float(line['x']))
			y.append(float(line['y']))
			z.append(float(line['z']))

	# plot it
	fig = plot.figure()
	ax = fig.add_subplot(111)
	ax.scatter(x, y)
	plot.show()


if __name__ == "__main__":
	main()
