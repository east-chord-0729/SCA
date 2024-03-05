# Date: 2023.10.20.(Fri)
# Writer: Ju-Hwan Kim (SICADA, KMU)

import numpy as np
import matplotlib.pyplot as plt

trace = np.loadtxt('trace0.txt')
plt.plot(trace)
plt.xlim(0, len(trace))
plt.xlabel('Time', fontsize=14)
plt.ylabel('Power Consumption', fontsize=14)
plt.show()