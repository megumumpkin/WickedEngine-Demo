#region Modules

import argparse
import os
import subprocess 
from sys import platform
import warnings

#endregion

#region Arguments

argumentParser = argparse.ArgumentParser(description="The arguments for the Python build script.")
argumentParser.add_argument('-p', '--play', dest = 'playGame', default = "False", help = "Whether to run the game or not once built.")
arguments = argumentParser.parse_args()

#endregion

#region Functions

def Build(): 
    # GNU/Linux
    if platform == "linux" or platform == "linux2":
        subprocess.run(['sh', 'Build.sh'], shell=False)
    
    # macOS & Darwin
    elif platform == "darwin":
        subprocess.run(['sh', 'Build.sh'], shell=False)

    # Windows
    elif platform == "win32":
        warnings.warn("Script not designed for Windows!")
        subprocess.run(['sh', 'Build.sh'], shell=False)


def Play():
    print("Running the Wicked Engine demo!")
    os.system('cd ../../Build/ && ./Game')

#endregion

#region Launch

if __name__ == "__main__":
    Build()

    if arguments.playGame == "true" or "True":
        Play()

#endregion