#!/usr/bin/env python3

import re
import click


class ThinkBat:

    def __init__(self, bat_num=0):
        self._path = "/sys/class/power_supply/BAT" + str(bat_num)
        self._stats = {}
        self.read_stats()

    def _thresh_type_str(self, is_start):
        return "start" if is_start else "end"

    def _thresh_key(self, is_start):
        return self._thresh_type_str(is_start) + "_threshold"

    def _get_thresh_path(self, is_start):
        return self._path + "/charge_control_" + self._thresh_type_str(is_start) + "_threshold"

    def _format_line(self, line):
        return re.sub("\ |\n", "", line).lower()

    def read_stats(self):
        with open(self._path + "/uevent", "r") as f:
            for line in f.readlines():
                line = self._format_line(line)
                key, val = tuple(line.split("="))
                if val.isnumeric():
                    val = int(val)
                key = key.replace("power_supply_", "")
                self._stats[key] = val

        self._stats["capacity"] = 100 * self._stats["energy_full"] / self._stats["energy_full_design"]

        self._stats["percentage"] = 100 * self._stats["energy_now"] / self._stats["energy_full"]

        with open(self._path + "/voltage_now", "r") as f:
            self._stats["voltage"] = int(self._format_line(f.readline())) / 1000000

        def read_thresholds(is_start):
            with open(self._get_thresh_path(is_start), "r") as f:
                self._stats[self._thresh_key(is_start)] = int(self._format_line(f.readline()))

        read_thresholds(is_start=True)
        read_thresholds(is_start=False)

    def set_charge_thresholds(self, start, end):
        def write_threshold(value, is_start):
            try:
                with open(self._get_thresh_path(is_start), "w") as f:
                    f.write(str(value))
                    self._stats[self._thresh_key(is_start)] = value
                    return True
            except PermissionError:
                print("Changing thresholds requires sudo!")
            except Exception as e:
                print("Failed to change thresholds: " + str(e))
            return False

        if write_threshold(start, True) and write_threshold(end, False):
            print("Successfully changed charging thresholds.")

    def man_info(self):
        return "BAT Type: \n" + \
               "\tManufacturer: " + self._stats["manufacturer"] + "\n" + \
               "\tModel: " + self._stats["model_name"] + "\n" + \
               "\tSerial: " + str(self._stats["serial_number"]) + "\n" + \
               "\tTechnology: " + self._stats["technology"]

    def status_info(self):
        return "BAT Status:\n" + \
               "\tPercentage: " + str(round(self._stats["percentage"], 3)) + "%\n" \
               "\tState: -" + self._stats["status"] + "-\n" + \
               "\tCapacity: " + str(round(self._stats["capacity"], 3)) + "%\n" + \
               "\tCycle Count: #" + str(self._stats["cycle_count"]) + "\n" + \
               "\tVoltage: " + str(self._stats["voltage"]) + " V\n" + \
               "\tStart Thresh: " + str(self._stats["start_threshold"]) + "%\n" + \
               "\tStop Thresh: " + str(self._stats["end_threshold"]) + "%"


@click.command()
@click.option("-t", "--thresholds", nargs=2, type=(click.INT, click.INT), help="Set start and stop thresholds")
def prompt(thresholds):
    bat = ThinkBat()
    if thresholds:
        bat.set_charge_thresholds(thresholds[0], thresholds[1])
    else:
        print(bat.man_info())
        print(bat.status_info())


if __name__ == "__main__":
    prompt()
