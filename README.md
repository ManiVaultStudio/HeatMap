# HeatMap plugin ![Build Status](https://github.com/ManiVaultStudio/HeatMap/actions/workflows/build.yml/badge.svg?branch=master)

HeatMap view plugin for the [ManiVault](https://github.com/ManiVaultStudio/core) visual analytics framework.

Given a point and a corresponding cluster data set, this plugin shows a heatmap of the mean value of each point data dimension for all cluster.

```bash
git clone git@github.com:ManiVaultStudio/HeatMap.git
```

<p align="middle">
  <img src="https://github.com/ManiVaultStudio/HeatMap/assets/58806453/87ae510c-083f-40e0-a04b-c357d2cab601" align="middle" width="75%" /> </br>
  Heatmap representation of mean values per cluster and dimension of the <a href="https://archive.ics.uci.edu/dataset/53/iris">iris flower data</a>. On top, a dendrogram shows a hierarchical clustering of the manually created data clusters. 
  Cluster "Four" is selected.
</p>

## Features
- Resize each tile wrt to the standard deviation of the respective dimension values in a cluster
- Hierarchical clustering (using euclidean distance) with [clusterfck](https://github.com/harthur/clustering)
- Adjust the min and max values of the color map (via a menu accesible from the arrow the bottom), and change the colormap itself (via clicking the colormap)
