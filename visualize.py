import serial
import time
from threading import Thread

import numpy as np
from matplotlib.lines import Line2D
import matplotlib.pyplot as plt
import matplotlib.animation as animation

PORT = '/dev/cu.usbmodemfa1341'
BAUD = 115200
BINS = 128
SMOOTH = .5
RUNNING_AVG_SMOOTH = .99

class PeriodicLogger(object):
    def __init__(self, period_seconds):
        self.period_seconds = period_seconds
        self.last_log = 0

    def log(self, msg):
        if time.time() - self.last_log > self.period_seconds:
            print msg
            self.last_log = time.time()

class SecondLogger(object):
    def __init__(self, tracking, period_seconds):
        self.tracking = tracking
        self.period_seconds = period_seconds
        self.last_output = time.time()
        self.count = 1

    def ping(self):
        self.count += 1
        duration = time.time() - self.last_output
        if duration > self.period_seconds:
            print('{}: {:.2f}/s'.format(self.tracking, self.count / duration))
            self.last_output = time.time()
            self.count = 0

class Sampler(object):
    def __init__(self, serial_port, baud):
        print 'Opening port'
        self.serial = serial.Serial(serial_port, baud)
        print 'Port opened'
        self.lines_read = SecondLogger('Lines Read', 5)

    def read_data(self):
        while True:
            serial_data = self.serial.readline()
            # Didn't get a whole line
            if not serial_data.startswith('start'):
                continue
            try:
                parsed = [int(v) for v in serial_data.split(',')[1:]]
            except Exception as e:
                print 'Bad line: {}'.format(e)
                continue
            if len(parsed) != BINS:
                print 'Bad read. {} entries'.format(len(parsed))
                continue
            self.lines_read.ping()
            return np.array(parsed, dtype=np.int)

    def calibrate(self, seconds):
        print 'Calibrating for {} seconds'.format(seconds)
        end = time.time() + seconds
        running_sum, i = self.read_data(), 1
        while time.time() < end:
            self.data = self.read_data()
            running_sum += self.data
            i += 1
        print 'Calibrated over {} samples'.format(i)
        self.average = running_sum / i
        print 'Average:\n{}'.format(self.average)


class NearMaxTracker(object):
    def __init__(self):
        self.last_max_index = None
        self.last_max_repeat = 0
        self.max_logger = PeriodicLogger(2)

    def repeat_to_zero(self):
        if self.last_max_repeat > 3:
            print 'Got {} repeats around max {}'.format(self.last_max_repeat, self.last_max_index)
        self.last_max_repeat = 0
        self.last_max_index = None

    def feed(self, max_val, max_index):
        self.max_logger.log('Max: {} Index: {}'.format(max_val, max_index))
        if max_val < 16 or max_index > 20:
            self.repeat_to_zero()
            return
        if self.last_max_index is not None:
            if abs(max_index - self.last_max_index) <= 2:
                self.last_max_repeat += 1
                if self.last_max_repeat > 4:
                    print 'Max index: {}'.format(self.last_max_index)
            else:
                self.repeat_to_zero()
        self.last_max_index = max_index

class Chart(object):
    def __init__(self, ax, sampler):
        self.ax = ax
        self.sampler = sampler
        self.max_tracker = NearMaxTracker()

        self.xdata = np.linspace(0, 20000, BINS)
        d = self.sampler.read_data()
        self.raw_data = d
        self.running_avg = d

        self.line = Line2D(self.xdata, self.raw_data)
        self.max_line = Line2D(np.array([0,0]),
                               np.array([0,244]), color='red')

        self.ax.add_line(self.line)
        self.ax.add_line(self.max_line)

        self.ax.set_ylim(-.1, 256)
        self.ax.set_xlim(0, 20000)

    def update(self, unused_param):
        new_data = self.sampler.read_data()
        self.running_avg = (self.running_avg * RUNNING_AVG_SMOOTH
                            + (1 - RUNNING_AVG_SMOOTH) * new_data)
        self.raw_data = self.raw_data * SMOOTH +  (1 - SMOOTH) * new_data
        transformed = np.clip(self.raw_data - self.running_avg, 0, 256)
        self.line.set_data(self.xdata, transformed)
        max_bucket = np.argmax(transformed)
        max_val = self.raw_data[max_bucket]
        self.max_tracker.feed(max_val, max_bucket)
        self.max_line.set_data(np.array([max_val, max_val]),
                               np.array([0,244]))
        return self.line, self.max_line



def main():
    sampler = Sampler(PORT, BAUD)

    print 'Example data:\n{}'.format(sampler.read_data())

    fig, ax = plt.subplots()
    chart = Chart(ax, sampler)

    anim = animation.FuncAnimation(fig, chart.update, interval=10,
                                   blit=False)
    plt.show()

if __name__ == '__main__':
    main()
