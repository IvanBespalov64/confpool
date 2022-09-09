#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

#include <iostream>
#include <vector>
#include <math.h>
#include <numeric>
#include <fstream>

#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#include "fmt/core.h"
#include "fmt/args.h"

#include "confpool.h"
#include "molproxy.h"
#include "rmsd.h"
#include "utils.h"

namespace py = pybind11;


void Confpool::include_from_file(const py::str& py_filename) {
    const auto filename = py_filename.cast<std::string>();
    include(filename);
}

void Confpool::include(const std::string& filename) {
    auto mylines = Utils::readlines(filename);
    // for (int i = 0; i < mylines.size(); ++i)
        // fmt::print("{} -- {}\n", i, mylines[i]);
    int cline = 0;
    while (cline < mylines.size()) {
        // std::cout << "Casting to int '" << mylines[cline] << "'" << "\n";
        boost::algorithm::trim(mylines[cline]);
        const unsigned int cur_natoms = boost::lexical_cast<int>(mylines[cline].c_str());
        if (natoms == 0) 
            natoms = cur_natoms;
        else if (natoms != cur_natoms) 
            throw std::runtime_error("Wrong numer of atoms");
        
        auto description = mylines[cline + 1];

        auto geom = CoordContainerType(natoms);
        SymVector atom_types;
        for (unsigned int i = 2; i < natoms + 2; ++i)
        {
            if (cline + i >= mylines.size())
                throw std::runtime_error(fmt::format("Unexpected number of atoms (expected {}, nlines={}). Check {}", natoms, mylines.size(), filename));
            
            std::vector<std::string> parts;
            boost::algorithm::trim(mylines[cline + i]);
            boost::split(parts, mylines[cline + i], boost::is_any_of(" "), boost::token_compress_on);
            if(parts.size() != 4)
                throw std::runtime_error("Unexpected number of parts in line. Check " + filename);
            
            atom_types.push_back(parts[0]);
            geom.set_atom(i - 2, {boost::lexical_cast<double>(parts[1].c_str()),
                                  boost::lexical_cast<double>(parts[2].c_str()),
                                  boost::lexical_cast<double>(parts[3].c_str())});
        }
        
        if (sym_.size() == 0)
            sym_ = atom_types;
        else if (sym_ != atom_types)
            throw std::runtime_error("Unexpected atom types. Check " + filename);
        
        coord_.push_back(geom);
        descr_.push_back(description);
        cline += 2 + natoms;
    }
    resize();
}

void Confpool::key_from_description(const py::str& py_keyname, const py::function& py_parser) {
    full_check();

    const auto keyname = py_keyname.cast<std::string>();
    // if (keys_.find(key) == keys_.end()) SOME OPTIMIZATIONS ARE POSSIBLE
    keys_[keyname] = std::vector<double>(coord_.size());
    for(int i = 0; i < coord_.size(); ++i)
        keys_[keyname][i] = py_parser(py::cast(descr_[i])).cast<double>();
}

void Confpool::distance_to_key(const py::str& py_keyname, const py::int_& py_idxA, const py::int_& py_idxB) {
    full_check();

    const auto keyname = py_keyname.cast<std::string>();
    const auto a_idx = py_idxA.cast<int>() - 1;
    const auto b_idx = py_idxB.cast<int>() - 1;
    if (a_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxA.cast<int>(), natoms));
    if (b_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxB.cast<int>(), natoms));

    keys_[keyname] = std::vector<double>(coord_.size());
    for(int i = 0; i < coord_.size(); ++i) {
        keys_[keyname][i] = coord_[i].get_distance(a_idx, b_idx);
    }
}

void Confpool::vangle_to_key(const py::str& py_keyname, const py::int_& py_idxA, const py::int_& py_idxB, const py::int_& py_idxC) {
    full_check();

    const auto keyname = py_keyname.cast<std::string>();
    const auto a_idx = py_idxA.cast<int>() - 1;
    const auto b_idx = py_idxB.cast<int>() - 1;
    const auto c_idx = py_idxC.cast<int>() - 1;
    if (a_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxA.cast<int>(), natoms));
    if (b_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxB.cast<int>(), natoms));
    if (c_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxC.cast<int>(), natoms));

    keys_[keyname] = std::vector<double>(coord_.size());
    for(int i = 0; i < coord_.size(); ++i)
        keys_[keyname][i] = coord_[i].get_vangle(a_idx, b_idx, c_idx);
}

void Confpool::dihedral_to_key(const py::str& py_keyname, const py::int_& py_idxA, const py::int_& py_idxB, const py::int_& py_idxC, const py::int_& py_idxD) {
    full_check();

    const auto keyname = py_keyname.cast<std::string>();
    const auto a_idx = py_idxA.cast<int>() - 1;
    const auto b_idx = py_idxB.cast<int>() - 1;
    const auto c_idx = py_idxC.cast<int>() - 1;
    const auto d_idx = py_idxD.cast<int>() - 1;
    if (a_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxA.cast<int>(), natoms));
    if (b_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxB.cast<int>(), natoms));
    if (c_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxC.cast<int>(), natoms));
    if (d_idx > natoms)
        throw std::runtime_error(fmt::format("Atom index {} is out of range (n_atoms = {})", py_idxD.cast<int>(), natoms));

    keys_[keyname] = std::vector<double>(coord_.size());
    for(int i = 0; i < coord_.size(); ++i)
        keys_[keyname][i] = coord_[i].get_dihedral(a_idx, b_idx, c_idx, d_idx);
}

void Confpool::filter(const py::function& py_criterion) {
    full_check();

    unsigned int del_count = 0;
    for(int i = coord_.size() - 1; i >= 0; --i) {
        if (!py_criterion(proxies_[i]).cast<bool>()) {
            remove_structure(i);
            del_count += 1;
        }
    }
    fmt::print("Deleted {} structures\n", del_count);
    resize();
}

py::int_ Confpool::count(const py::function& py_criterion) {
    full_check();

    unsigned int res = 0;
    for(int i = coord_.size() - 1; i >= 0; --i)
        if (py_criterion(proxies_[i]).cast<bool>())
            res += 1;
    return res;
}

void Confpool::remove_structure(const int& i) {
    auto c_it = coord_.begin();
    std::advance(c_it, i);
    coord_.erase(c_it);

    auto d_it = descr_.begin();
    std::advance(d_it, i);
    descr_.erase(d_it);

    for(auto& pair : keys_) { // pair = key, std::vector<double>
        auto it = pair.second.begin();
        std::advance(it, i);
        pair.second.erase(it);
    }
}

void Confpool::resize() {
    if (coord_.size() != descr_.size())
        throw std::runtime_error(fmt::format("Mismatch of container sizes (coord and descr): {} vs. {}", coord_.size(), descr_.size()));
    
    for(auto& pair : keys_) { // pair = key, std::vector<double>
        if (pair.second.size() > coord_.size())
            throw std::runtime_error(fmt::format("Mismatch of container sizes (key container > coord): {} < {}", pair.second.size(), coord_.size()));
        else if (pair.second.size() < coord_.size())
            pair.second.resize(coord_.size());
    }
    
    if (proxies_.size() != coord_.size()) {
        proxies_.resize(coord_.size());
        for (int i = 0; i < coord_.size(); ++i)
            proxies_[i] = MolProxy(this, i);
    }
}

void Confpool::delete_by_proxy(const MolProxy& mol) {
    delete_by_idx(mol.get_index());
}

void Confpool::update_description(const py::function& descr_f) {
    full_check();

    for(auto i = 0; i < coord_.size(); ++i)
        descr_[i] = descr_f(proxies_[i]).cast<std::string>();
}

void Confpool::upper_cutoff(const py::str& py_keyname, const py::float_& py_cutoff) {
    full_check();
    const auto cutoff = py_cutoff.cast<double>();
    if (cutoff <= 0.0)
        throw std::runtime_error(fmt::format("Cutoff value must be > 0. {} given.", cutoff));

    const auto& key_data = keys_[py_keyname.cast<std::string>()];

    auto minimal_value = key_data[0];
    for (const auto& current_val : key_data)
        if (current_val < minimal_value)
            minimal_value = current_val;
    
    unsigned int del_count = 0;
    for(int i = coord_.size() - 1; i >= 0; --i) {
        if (key_data[i] - minimal_value > cutoff) {
            remove_structure(i);
            del_count += 1;
        }
    }
    fmt::print("Deleted {} structures\n", del_count);
    resize();
}

void Confpool::lower_cutoff(const py::str& py_keyname, const py::float_& py_cutoff) {
    full_check();
    const auto cutoff = py_cutoff.cast<double>();
    if (cutoff <= 0.0)
        throw std::runtime_error(fmt::format("Cutoff value must be > 0. {} given.", cutoff));

    const auto& key_data = keys_[py_keyname.cast<std::string>()];

    auto maximal_value = key_data[0];
    for (const auto& current_val : key_data)
        if (maximal_value < current_val)
            maximal_value = current_val;
    
    unsigned int del_count = 0;
    for(int i = coord_.size() - 1; i >= 0; --i) {
        if (maximal_value - key_data[i] > cutoff) {
            remove_structure(i);
            del_count += 1;
        }
    }
    fmt::print("Deleted {} structures\n", del_count);
    resize();
}

void Confpool::save(const py::str& py_filename) {
    full_check();
    const auto filename = py_filename.cast<std::string>();

    auto natoms_str = boost::lexical_cast<std::string>(natoms);
    std::vector<std::string> reslines;
    for(auto i = 0; i < coord_.size(); ++i) {
        reslines.push_back(natoms_str);
        reslines.push_back(descr_[i]);

        for(auto j = 0; j < natoms; ++j) {
            const auto& coords = coord_[i].get_atom(j);
            reslines.push_back(fmt::format("{:>2}  {:12.8f}  {:12.8f}  {:12.8f}", sym_[j], coords[0], coords[1], coords[2]));
        }
    }

    auto joined = boost::algorithm::join(reslines, "\n");
    std::ofstream out(filename);
    out << joined << "\n\n\n";
    out.close();
}

void Confpool::full_check() const {
    if (coord_.size() != descr_.size())
        throw std::runtime_error(fmt::format("Mismatch of container sizes (coord and descr): {} vs. {}", coord_.size(), descr_.size()));
    
    for(auto& pair : keys_) { // pair = key, std::vector<double>
        if (pair.second.size() != coord_.size())
            throw std::runtime_error(fmt::format("Mismatch of container sizes (key='{}' container > coord): {} < {}", pair.first, pair.second.size(), coord_.size()));
    }

    if (coord_.size() != proxies_.size())
        throw std::runtime_error(fmt::format("Mismatch of container sizes (coord and proxy container): {} vs. {}", coord_.size(), proxies_.size()));
    
    for (int i = 0; i < coord_.size(); ++i)
        if (proxies_[i].get_index() != i)
            throw std::runtime_error(fmt::format("MolProxy #{} has number {} (must be equal)", i, proxies_[i].get_index()));
}

void Confpool::sort(const py::str& py_keyname) {
    full_check();
    std::cout << "Doing some sorting" << std::endl;
    
    const auto keyname = py_keyname.cast<std::string>();
    auto p = Utils::sort_permutation(keys_[keyname]);
    std::cout << "Reorder = " << py::repr(py::cast(p)).cast<std::string>() << std::endl;

    Utils::apply_permutation_in_place(descr_, p);
    Utils::apply_permutation_in_place(coord_, p);
    for(auto& pair : keys_) // pair = key, std::vector<double>
        Utils::apply_permutation_in_place(pair.second, p);
}

void Confpool::rmsd_filter(const py::float_& py_rmsd_cutoff) {
    full_check();

    unsigned int del_count = 0;
    const auto cutoff = py_rmsd_cutoff.cast<double>();
    const auto atom_ints = Utils::generate_atom_ints(sym_);
    RmsdCalculator rmsd(natoms, atom_ints);
    for (int i = coord_.size() - 1; i > 0; i--) {
        auto& curgeom = coord_[i].to_boost_format();
        for (int j = i - 1; j >= 0; j--) {
            auto& testgeom = coord_[j].to_boost_format();
            if (rmsd.calc(curgeom, testgeom) < cutoff) {
                remove_structure(i);
                del_count += 1;
                break;
            }
        }
    }
    fmt::print("Deleted {} structures\n", del_count);
    resize();
}

MolProxy Confpool::__getitem__(const py::int_& idx)
{ return proxies_[idx]; }

/*
py::dict Confpool::get_structure(const py::int_& py_index) {
    const auto index = py_index.cast<int>();
    if (index < 0) {
        throw std::runtime_error(fmt::format("Given index ({}) is less than zero", index));
    }
    else if (index >= coord_.size()) {
        throw std::runtime_error(fmt::format("Given index ({}) is bigger than the maximum list element ({})", index, coord_.size()));
    }

    py::dict res;

    if (index < ener_.size())
        res["energy"] = py::cast(ener_[index]);
    res["descr"] = py::cast(descr_[index]);

    const size_t size = natoms * 3;
    double *coord_array = new double[size];
    for (size_t i = 0; i < natoms; i++) {
        const auto& coords = coord_[index].get_atom(i);
        coord_array[i * 3 + 0] = coords[0];
        coord_array[i * 3 + 1] = coords[1];
        coord_array[i * 3 + 2] = coords[2];
    }

    py::capsule free_when_done(coord_array, [](void *f) {
        double *foo = reinterpret_cast<double *>(f);
        delete[] foo;
    });

    res["xyz"] = py::array_t<double>(
        {static_cast<int>(natoms), 3}, // shape
        {3*8, 8}, // C-style contiguous strides for double
        coord_array, // the data pointer
        free_when_done);
    return res;
}
*/