# Tutorial: selection of the most informative FRET pairs
## Summary
In this tutorial we walk through the procedure of selecting an informative set of FRET-pairs. In this context we define set as informative if it allows to distinguish any single conformation from a set of hypothetical structures (prior) using the FRET measurements. The software looks for such a set of FRET pairs, that expected structural precision is maximized with as little FRET pairs as possible. More information is available in the "Quantitative FRET" article[![DOI for citing LabelLib](https://img.shields.io/badge/DOI-10.1016%2Fj.sbi.2016.11.012-blue.svg)](https://doi.org/10.1016/j.sbi.2016.11.012):
> Dimura, M., Peulen, T.O., Hanke, C.A., Prakash, A., Gohlke, H. and Seidel, C.A., 2016. Quantitative FRET studies and integrative modeling unravel the structure and dynamics of biomolecular systems. Current opinion in structural biology, 40, pp.163-185.

We use T4 lysozyme (T4L) protein as an example. It is recommended go through the [screening tutorial](/doc/screening%20tutorial/screening%20tutorial.md) first in order to get familiar with the software features. This tutorial consists of 7 steps:

1. Start the software
2. Load an ensemble of conformations
3. Create a reference label
4. Configure the linker and dye parameters for the reference label
5. Create labeling positions for each residue in the molecule
6. Create FRET pairs for all possible pairwise combinations of labels
7. Find 10 most informative FRET pairs

## FRET pair selection
1. Start Olga software (`Olga.exe` executable).

2. Choose "Import trajectory" menu item to load a trajectory. Select the trajectory and topology files you need to screen. In this example we use a [trajectory](../data/T4L/3GUN_NMSim_cl-rep_894.dcd) and the corresponding [topology](../data/T4L/3GUN_NMSim_cl-rep-001.pdb), which contains structural models of T4L protein generated by [NMSim](http://nmsim.de/) software from the 3GUN crystal structure. The trajectory was clustered using RMSD as the distance metric and a threshold of 1.8 Angstrom. 

     ![Import trajectory](import_trajectory.png)
     
     ![Specify trajectory files](specify_trajectory_files.png)
   
3. Now we need to create a labelling position evaluator that will serve as a template for other positions. In this tutorial we use the same template for donor and acceptor labels, but this is not mandatory. Select "Position" option from the dropdown menu, then press the "+" button.

     ![Create Evaluator](create%20evaluator.png)

     New labelling position will appear in the evaluators panel. Unfold it to see its settings.

4. Now Labelling Position properties can be filled out. In this tutorial parameters are chosen to represent both Alexa 488 and Alexa 647 dyes. First one has to choose the simulation type (here AV1 is selected). Chromophore moiety approximated by a sphere with the radius of 3.5 Angstrom (`radius1 = 3.5`). It's linker has length of 21 Angstrom at maximum extension (`linker_length = 21`). We use linker width of 2 Angstrom; digitization step is set to 0.9 Angstrom (`simulation_grid_resolution = 0.9`). For the template chain ID (`chain_identifier`), residue ID (`residue_seq_number`), residue name (`residue_name`) should not be changed, they will be set automatically later.  In experiment, typically, labelled residue is mutated to cysteine and then the dye is covalently bound to it by maleimide linker. To mimic that, we use Cβ atom as the attachment point (`atom_name = CB`). User can specify `allowed_sphere_radius` option, which tells algorithm to ignore obstacles in the given radius in AV simulation. For example, side chain atoms would not exist in experiment, but they are present in the PDB files and can be disregarded for AV simulations.
    We can rename evaluator from default `new LP`  to, e.g. `template_LP` by double-clicking on its name. Activate the template labelling position draft by pressing "save" button.  
    
    ![Template labelling position settings](template%20LP.png)

5. Next, we will use `Wizards -> Add multiple labeling positions` from the top menu bar to automatically populate the labelling positions list with all residues of interest. Normally one would select almost all positions in this dialog. Positions can be omitted if corresponding mutations could affect structure or function of the protein or for any other reason. For the sake of demonstration and to save time we include only residues 35-50, 85-95 and 115-120. Time needed to calculate FRET efficiencies is proportional to the number of pairs added (next step).
   
    ![Add labelling positions menu](add%20LPs.png)

    You can use `Ctrl + A` combination to select all residues and `Space` to toggle the check boxes.

    ![Add labelling positions dialog](add%20LPs%20dialog.png)

      Once all needed residues are checked, press OK button. Positions will appear in the Evaluators widget:

    ![Add labelling positions dialog](all%20positions.png)

    `template_LP` is not needed anymore and can be removed using "delete" button.

6. Next, we add FRET efficiency evaluators for all possible pairwise combinations of labeling positions. Go to `Wizards -> Add multiple FRET efficiencies` to show the corresponding dialog.  Set the Förster radius, then check labelling positions you want to be included in the ranking (we select all), and press OK. It may take a couple of minutes to add all FRET pairs. Once pairs are added, the program will start calculating efficiencies for each loaded conformer, which might take about 30 min.

   ![Add FRET efficiencies dialog](add%20efficiencies.png)

   ![Efficiency evaluator addition finished](all%20efficiencies.png)

7. Once all FRET efficiencies are calculated, we can rank them by how informative they could be. Go to `Wizards -> Determine informative labeling pairs` to show the pair selection dialog.

   ![Pair selection dialog](pair%20selection%20dialog.png)

   Enter the number of pairs you would like to select, atoms you would like to consider for RMSD calculation and expected error in FRET efficiency. Optionally, it is possible to specify a file where the results should be saved. If no results file is selected, pair list will be printed in the "Information log" widget. Press OK to start the calculation.

   ![Pair selection dialog](pair%20selection%20results.png)
