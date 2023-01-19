# 🕹️ Unreal part

<details>
  <summary>📁 Table of Contents</summary>
  <ol>
    <li>
      <a href="#general">General</a>
    </li>
    <li>
      <a href="#requirements">Requirements</a>
    </li>
    <li><a href="#configuration">Configuration</a></li>
    <li><a href="#contributors">Contributors</a></li>
  </ol>
</details>

### General

This repository generates __synthetic data of Paris__ from [OpenStreetMap](https://www.openstreetmap.org) data and animates it with the [City Sample](https://www.unrealengine.com/marketplace/en-US/product/city-sample) project (pedestrians, cars). 
The synthetic data is captured in several formats (colour / depth / normal / optical / semantic images).<br/>

The objective is to train AI models from this synthetic data, for example to detect windows to be blurred.

### Requirements

- [Unreal Engine 5.1](https://www.unrealengine.com/fr/unreal-engine-5)

In your project, you will need to add:
- [City Sample Crowds](https://www.unrealengine.com/marketplace/en-US/product/city-sample-crowds)
- [City Sample Vehicles](https://www.unrealengine.com/marketplace/en-US/product/city-sample-vehicles)

Create a Plugins folder at the root of your project and add the correct version of [EasySynth](https://github.com/ydrive/EasySynth/tree/ue5.1).

### Configuration

For __City Sample__, this is a short [tutorial](https://www.youtube.com/watch?v=2LvUB3_PAhI) to help you understand how to install and use city sample in your project. <br/>
To capture our synthetic data we use __EasySynth__, which you can find the configuration in [this tutorial](https://www.reddit.com/r/MachineLearning/comments/s2yvyk/n_easysynth_unreal_engine_plugin_for_easy/).

### Contributors

- Lead Dev - [Jehanne Dussert](https://github.com/JehanneDussert/)
- Dev - [Rémi Giner](https://github.com/remisansfamine/)
- Dev - [Nobila Traore](https://github.com/notraore/)
- Game Artist - [William Lahemar](https://github.com/willocks)
