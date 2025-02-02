from xml.etree.ElementTree import Element, SubElement, ElementTree, dump
from xml.etree.ElementTree import parse
import glob
import os
from lxml import etree
import pandas as pd
import argparse
import numpy as np
from easydict import EasyDict as edict
import math
from tqdm import tqdm

from pymap3d.ecef import ecef2geodetic
from pymap3d.ned import ned2geodetic, geodetic2ned, geodetic2enu

import csv

HEADER = '''\
# .PCD v0.7 - Point Cloud Data file format
VERSION 0.7
FIELDS x y z intensity
SIZE 4 4 4 4 
TYPE F F F F 
COUNT 1 1 1 1 
WIDTH {}
HEIGHT 1
VIEWPOINT 0 0 0 1 0 0 0
POINTS {}
DATA ascii
'''

def indent(elem, level=0): 
    i = "\n" + level*"  "
    if len(elem):
        if not elem.text or not elem.text.strip():
            elem.text = i + "  "
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
        for elem in elem:
            indent(elem, level+1)
        if not elem.tail or not elem.tail.strip():
            elem.tail = i
    else:
        if level and (not elem.tail or not elem.tail.strip()):
            elem.tail = i

def write_pcd(points, save_pcd_path):
    n = len(points)
    lines = []
    for i in range(n):
        # print(points[i])
        x, y, z, i = points[i]
        lines.append('{:.6f} {:.6f} {:.6f} {}'.format(x, y, z, i))

    with open(save_pcd_path, 'w') as f:
        f.write(HEADER.format(n, n))
        f.write('\n'.join(lines))

def write_csv(points, save_csv_path):
    header = ['x', 'y','z','idx']

    with open(save_csv_path, 'w', encoding='UTF8') as f:
        writer = csv.writer(f)

        # write the header
        writer.writerow(header)
        for point in points:
            # write the data
            writer.writerow(point)

def parse_configs():
    parser = argparse.ArgumentParser(description='The csv data read')
    parser.add_argument('--filename', type=str, default='',
                        help='csv file name')
    parser.add_argument('--check_header', type=bool, default=False,
                        help='check headers name')
    parser.add_argument('--set_topic', type=str, default="/novatel_bottom/bestpos",
                        help='check headers name')

    lon_0, lat_0 = -86.235148, 39.809786  #indy
    # lon_0, lat_0 = -86.3418060783425, 39.8125900071711  #Lucas Oil Racing
    
    nedPoints = []

    configs = edict(vars(parser.parse_args()))

    root_path = './include' 
    osm_file_name = os.path.join(root_path, 'osm', 'ims' ,configs.filename)
    pcd_file_name = osm_file_name[:-3] + "pcd" 
    wpt_file_name = osm_file_name[:-4] + "_wpt.csv" 

    # osm_file_name = os.path.join(root_path, osm_file_name)



    tree = parse(osm_file_name)
    root = tree.getroot()

    nodes = root.findall("node")
    ways = root.findall("way")

    idx = 0
    node_map = {}
    for node in tqdm(nodes):

        lat = float(node.attrib['lat'])
        lon = float(node.attrib['lon'])

        ned = geodetic2ned(lat, lon, 0., lat_0, lon_0, 0., deg=True)
        map_buf = {int(node.attrib['id']): [ned[0], -ned[1], 0.0, 0.0]}        
        # if ((ned[0] - x_prev)**2 + (ned[1] - y_prev)**2) ** 0.5 < 0.5 :
        #     continue 
        node_map.update(map_buf)
        # print("ned: " , ned)

        # pointBuf = [ned[0], -ned[1], 0.0, idx]
        # nedPoints.append(pointBuf)
        # print(pointBuf)

    for way in ways:
        nds = way.findall("nd")

        for nd in nds:
            ref_nd = int(nd.attrib['ref'])
            
            pointBuf = node_map[ref_nd]
            pointBuf[-1] = idx
            print(pointBuf)
            nedPoints.append(pointBuf)
    
            idx = idx + 1


    # print(node_map)
    indent(root)
    # dump(root)            
    tree = ElementTree(root)
    tree.write(osm_file_name)
    write_pcd(nedPoints, pcd_file_name)
    write_csv(nedPoints, wpt_file_name)


    print("===========" * 5)
    print("COMPLETED!")


if __name__ == "__main__":
    parse_configs()
