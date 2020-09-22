# Tutorial: screening of structural models and generation of accessible volumes
## Summary
In this tutorial we walk through the calculation of FRET-average inter-dye distances for the case when fluorescent proteins are used as dyes. Fluorescent proteins are usually fused to the investigated protein, typically at the C- or N-terminus of the protein of interest. It is common to add a short linker/spacer sequence between the fluorescent protein's β-barrel and the protein under investigation. Here we assume, that such a sequence does not adopt a fixed conformation, but is instead disordered and dynamic, leading to a distribution of orientations of the fluorophore with respect to the rest of the macromolecule.

## Screening
1. Start Olga software

2. Press "Import structures" button to load a set of PDB files. Select the files that you need to screen, use shift/ctrl buttons to select multiple files. In this example we use a [PDB file](../data/TGR5/TGR5-GProt_dimer1-8_mem.pdb.gz) that contains a model of TGR5 protein embedded in a phospholipidic membrane (you will need to uncompress the file first).

![Import Structures](import%20structures.png)

3. Create a labelling position evaluator. To do this, you need to select "Position" from the dropdown menu and then press "+" button.

![Create Evaluator](create%20evaluator.png)

New labelling position will appear in the evaluators panel. Unfold it to see its settings.

![Show new LP](new%20LP%20settings.png)

4. Now Labelling Position properties can be filled out. First one has to choose the simulation type (here AV1 is selected). In this example Donor label is shown, GFP is used as a dye (radius1 = 20 Angstrom). It is attached by linker consisting of 62 aminoacids, therefore linker length at maximum extension is defined as linker_legth = 62 * 3.7 + 20 = 249.4, where 3.7 is the length of a single amino acid and 20 is the effective radius of the GFP. Here linker width of 7 Angstrom is used, digitization step is set to 2 Angstrom (simulation_grid_resolution = 2). You will need to specify chain id, residue id, residue name and atom name of the atom, to which fluorophore linker is attached. User can specify allowed_sphere_radius option, which tells algorithm to ignore obstacles in the given radius in AV simulation. Since in this example the dye (GFP) is attached to the protein by a peptide linker, chain_weighting option is activated to account for the polypeptide chain dynamics. This option reweights the probability density distribution within the AV as described by XXXXX The name of the labelling position can be changed by double-clicking on the corresponding field (here "A:ALA1088").

![Donor LP](donor%20LP.png)

5. Copy the Donor labelling position and modify the copy to represent the Acceptor.

![Acceptor LP](acceptor%20LP.png)

6. Activate Donor and Acceptor labelling position drafts by pressing "save" button.

![Save LPs](save%20LPs.png)

7. Create a distance evaluator. To do this select "Distance" from dropdown menu and press "+", as in step 3.

8. Select the distance type, donor and acceptor labelling position, specify the Förster radius. If available, you can specify the measured distance and error.

![Distance LP](distance%20LP.png)

9. Save the distance evaluator to start calculation. Once the distance evaluator is saved, a corresponding column will appear next to the structure list. At first this column will only show "...". Once the corresponding calculation is finished, calculated distance value will show up (or "nan" if the distance could not be calculated).

![Distance ready](distance%20ready.png)

10. Results table can be saved by pressing "Export calculated values" button.

![Save results](save%20results.png)

## Saving the Accessible Volume files
1. Create "AV file" evaluator. ![Create AV file](create%20AV%20file.png)

2. Select the labelling position, format (openDX or .pqr) and directory to save the files (write_dir). If write_dir is empty, directory of the PDB file is used. Once the AV File evaluator is saved, program will automatically simulate AVs and save AV clouds for each of the loaded PDBs.

![AV file settings](AV%20file%20settings.png)

3. AV cloud files can be now found in the write_dir. In this example .dx format is used. Here Pymol software is used to view the resulting cloud.

![Pymol volume](pymol%20volume.png)

![Pymol volume2](pymol%20volume2.png)
