import serial
import time

import numpy as np
from matplotlib.lines import Line2D
import matplotlib.pyplot as plt
import matplotlib.animation as animation

PORT = '/dev/cu.usbmodemfa1311'
BAUD = 115200
BINS = 128
SMOOTH = .8

class Sampler(object):

    def __init__(self, serial_port, baud):
        print 'Opening port'
        self.serial = serial.Serial(serial_port, baud)
        print 'Port opened'
        self.num_samples = 0

    def new_data(self):
        while True:
            serial_data = self.serial.readline()
            # Didn't get a whole line
            if not serial_data.startswith('start'):
                continue
            try:
                parsed = [int(v) for v in serial_data.split(',')[1:]]
            except Exception as e:
                print 'Bad line: {}'.format(e)
            if len(parsed) != BINS:
                print 'Bad read. {} entries'.format(len(parsed))
                continue
            self.num_samples += 1
            if self.num_samples % 100 == 0:
                print '{} samples returned'.format(self.num_samples)
            return np.array(parsed, dtype=np.int)

    def calibrate(self, seconds):
        print 'Calibrating for {} seconds'.format(seconds)
        end = time.time() + seconds
        running_sum, i = self.new_data(), 1
        while time.time() < end:
            running_sum += self.new_data()
            i += 1
        print 'Calibrated over {} samples'.format(i)
        self.average = running_sum / i
        print 'Average:\n{}'.format(self.average)


class Chart(object):
    def __init__(self, ax, sampler):
        self.ax = ax
        self.sampler = sampler

        self.xdata = np.linspace(0, 20000, BINS)
        self.raw_data = self.sampler.new_data()
        self.ydata = self.raw_data
        self.line = Line2D(self.xdata, self.ydata)
        self.ax.add_line(self.line)
        self.ax.set_ylim(-.1, 256)
        self.ax.set_xlim(0, 20000)

    def update(self, unused_param):
        self.raw_data = self.raw_data * SMOOTH + self.sampler.new_data() * (1 - SMOOTH)
        self.ydata = self.raw_data - self.sampler.average
        np.clip(self.ydata, 0, 256, out=self.ydata)
        self.line.set_data(self.xdata, self.ydata)
        return self.line,



def main():
    sampler = Sampler(PORT, BAUD)

    print 'Example data:\n{}'.format(sampler.new_data())

    sampler.calibrate(seconds=2)

    fig, ax = plt.subplots()
    chart = Chart(ax, sampler)

    anim = animation.FuncAnimation(fig, chart.update, interval=5,
                                   blit=False)

    plt.show()

if __name__ == '__main__':
    main()
