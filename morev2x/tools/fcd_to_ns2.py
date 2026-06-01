#!/usr/bin/env python3
"""
Convert SUMO FCD XML to ns-2 mobility trace format (.tcl).

Usage:
    python3 fcd_to_ns2.py --fcd-input <fcd.xml> --ns2-output <trace.tcl> [--step 0.01]

The ns-2 trace format:
    $node_(0) set X_ 123.45
    $node_(0) set Y_ 67.89
    $node_(0) set Z_ 0.00
    $ns_ at 1.0 "$node_(0) setdest 124.50 67.89 13.89"

Vehicle ID mapping: veh1 -> $node_(0), veh2 -> $node_(1), etc.
IDs are sorted numerically (veh1, veh2, ..., veh10, veh11, ...).

NOTE: Default --step changed to 0.01 to match VaN3Twin SUMO granularity (sumo_updates=0.01s).
This preserves full temporal resolution for MoReV2X Ns2MobilityHelper interpolation.
"""

import argparse
import xml.etree.ElementTree as ET
import re
import sys
import json
from collections import OrderedDict


def natural_sort_key(s):
    """Sort strings with embedded numbers naturally: veh1, veh2, ..., veh10."""
    return [int(c) if c.isdigit() else c.lower() for c in re.split(r'(\d+)', s)]


def convert_fcd_to_ns2(fcd_path, ns2_path, time_step=0.01, mapping_path=None):
    """
    Parse FCD XML and produce ns-2 mobility trace.
    
    Args:
        fcd_path: Path to SUMO FCD XML file
        ns2_path: Output path for ns-2 .tcl trace
        time_step: Resample FCD to this time step (seconds).
                   Default: 0.01s (10ms) — matches VaN3Twin SUMO granularity
                   (sumo_updates=0.01 in v2v-emergencyVehicleAlert-nrv2x.cc).
                   Use 0.1 for smaller output files when high resolution is not needed.
        mapping_path: Optional path to save vehID -> nodeIndex JSON mapping
    """
    print(f"Parsing FCD file: {fcd_path}")
    
    # --- Pass 1: collect all unique vehicle IDs ---
    all_veh_ids = set()
    context = ET.iterparse(fcd_path, events=('end',))
    for event, elem in context:
        if elem.tag == 'vehicle':
            all_veh_ids.add(elem.get('id'))
        if elem.tag == 'timestep':
            elem.clear()  # free memory
    
    # Sort naturally: veh1, veh2, ..., veh10, veh20
    sorted_ids = sorted(all_veh_ids, key=natural_sort_key)
    id_to_index = {vid: idx for idx, vid in enumerate(sorted_ids)}
    
    print(f"Found {len(sorted_ids)} unique vehicles: {sorted_ids[:5]}...{sorted_ids[-3:]}")
    print(f"Mapping: {dict(list(id_to_index.items())[:5])}")
    
    if mapping_path:
        with open(mapping_path, 'w') as mf:
            json.dump(id_to_index, mf, indent=2)
        print(f"Mapping saved to {mapping_path}")
    
    # --- Pass 2: resample and write ns-2 trace ---
    # We need initial positions and movement commands
    # Strategy: 
    #   - Record first appearance of each vehicle -> set initial X_, Y_, Z_
    #   - At each resampled timestep, emit setdest commands
    
    print(f"Resampling at {time_step}s step and writing ns-2 trace...")
    
    n_vehicles = len(sorted_ids)
    initial_pos = {}  # veh_id -> (x, y)
    prev_pos = {}     # veh_id -> (x, y, t)
    
    # Collect data at resampled intervals
    last_output_time = -1.0
    
    with open(ns2_path, 'w') as out:
        out.write(f"# ns-2 mobility trace converted from SUMO FCD\n")
        out.write(f"# Source: {fcd_path}\n")
        out.write(f"# Vehicles: {n_vehicles}\n")
        out.write(f"# Time step: {time_step}s\n\n")
        
        context = ET.iterparse(fcd_path, events=('end',))
        current_timestep_vehicles = {}
        current_time = None
        
        for event, elem in context:
            if elem.tag == 'vehicle':
                parent_time = current_time
                if parent_time is not None:
                    vid = elem.get('id')
                    x = float(elem.get('x'))
                    y = float(elem.get('y'))
                    speed = float(elem.get('speed', '0'))
                    current_timestep_vehicles[vid] = (x, y, speed)
                    
            elif elem.tag == 'timestep':
                t = float(elem.get('time'))
                current_time = t
                
                # Process previous timestep's vehicles if we have them
                # and it's time to output (resampled)
                if current_timestep_vehicles and t > 0:
                    process_time = t  # the time we just finished
                    
                    # Check if we should output at this time
                    if last_output_time < 0 or (process_time - last_output_time) >= time_step - 0.001 * time_step:
                        for vid, (x, y, speed) in current_timestep_vehicles.items():
                            idx = id_to_index[vid]
                            
                            if vid not in initial_pos:
                                # First appearance - set initial position
                                initial_pos[vid] = (x, y)
                                out.write(f'$node_({idx}) set X_ {x:.2f}\n')
                                out.write(f'$node_({idx}) set Y_ {y:.2f}\n')
                                out.write(f'$node_({idx}) set Z_ 0.00\n')
                            
                            # Emit setdest command
                            # Speed for setdest: we use actual speed from FCD
                            if speed < 0.01:
                                speed = 0.01  # avoid zero speed in setdest
                            out.write(f'$ns_ at {process_time:.2f} "$node_({idx}) setdest {x:.2f} {y:.2f} {speed:.2f}"\n')
                            
                            prev_pos[vid] = (x, y, process_time)
                        
                        last_output_time = process_time
                
                current_timestep_vehicles = {}
                elem.clear()
        
        # Handle vehicles that never appeared - place them far away
        for vid in sorted_ids:
            if vid not in initial_pos:
                idx = id_to_index[vid]
                out.write(f'$node_({idx}) set X_ -10000.00\n')
                out.write(f'$node_({idx}) set Y_ -10000.00\n')
                out.write(f'$node_({idx}) set Z_ 0.00\n')
    
    print(f"Done. Output: {ns2_path}")
    print(f"  Vehicles with data: {len(initial_pos)}/{n_vehicles}")
    return id_to_index


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description='Convert SUMO FCD XML to ns-2 mobility trace')
    parser.add_argument('--fcd-input', required=True, help='Path to SUMO FCD XML file')
    parser.add_argument('--ns2-output', required=True, help='Output path for ns-2 .tcl trace')
    parser.add_argument('--step', type=float, default=0.01, help='Resample time step in seconds (default: 0.01, matches VaN3Twin SUMO granularity)')
    parser.add_argument('--mapping', default=None, help='Save vehicle ID mapping to JSON file')
    
    args = parser.parse_args()
    convert_fcd_to_ns2(args.fcd_input, args.ns2_output, args.step, args.mapping)
