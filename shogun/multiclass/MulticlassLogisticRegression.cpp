/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Written (W) 2012 Sergey Lisitsyn
 * Copyright (C) 2012 Sergey Lisitsyn
 */


#include <shogun/multiclass/MulticlassLogisticRegression.h>
#include <shogun/multiclass/MulticlassOneVsRestStrategy.h>
#include <shogun/io/SGIO.h>
#include <shogun/mathematics/Math.h>
#include <shogun/labels/MulticlassLabels.h>
#include <shogun/lib/slep/slep_mc_plain_lr.h>

#include <utility>

using namespace shogun;

MulticlassLogisticRegression::MulticlassLogisticRegression() :
	LinearMulticlassMachine()
{
	init_defaults();
	register_parameters();
}

MulticlassLogisticRegression::MulticlassLogisticRegression(float64_t z, const std::shared_ptr<DotFeatures>& feats, std::shared_ptr<Labels> labs) :
	LinearMulticlassMachine(std::make_shared<MulticlassOneVsRestStrategy>(),feats,NULL,std::move(labs))
{
	init_defaults();
	register_parameters();
	set_z(z);
}

void MulticlassLogisticRegression::init_defaults()
{
	set_z(0.1);
	set_epsilon(1e-2);
	set_max_iter(10000);
}

void MulticlassLogisticRegression::register_parameters()
{
	SG_ADD(&m_z, "m_z", "regularization constant", ParameterProperties::HYPER);
	SG_ADD(&m_epsilon, "m_epsilon", "tolerance epsilon");
	SG_ADD(&m_max_iter, "m_max_iter", "max number of iterations");
}

MulticlassLogisticRegression::~MulticlassLogisticRegression()
{
}

bool MulticlassLogisticRegression::train_machine(std::shared_ptr<Features> data)
{
	if (data)
		set_features(data->as<DotFeatures>());

	require(m_features, "No features attached!");
	require(m_labels, "No labels attached!");
	require(m_multiclass_strategy, "No multiclass strategy"
			" attached!");

	auto mc_labels = multiclass_labels(m_labels);
	int32_t n_classes = mc_labels->get_num_classes();
	int32_t n_feats = m_features->get_dim_feature_space();

	slep_options options = slep_options::default_options();
	if (!m_machines.empty())
	{
		SGMatrix<float64_t> all_w_old(n_feats, n_classes);
		SGVector<float64_t> all_c_old(n_classes);
		for (int32_t i=0; i<n_classes; i++)
		{
			auto machine = m_machines.at(i)->as<LinearMachine>();
			SGVector<float64_t> w = machine->get_w();
			for (int32_t j=0; j<n_feats; j++)
				all_w_old(j,i) = w[j];
			all_c_old[i] = machine->get_bias();
		}
		options.last_result = new slep_result_t(all_w_old,all_c_old);
		m_machines.clear();
	}
	options.tolerance = m_epsilon;
	options.max_iter = m_max_iter;
	slep_result_t result = slep_mc_plain_lr(m_features,mc_labels,m_z,options);

	SGMatrix<float64_t> all_w = result.w;
	SGVector<float64_t> all_c = result.c;
	for (int32_t i=0; i<n_classes; i++)
	{
		SGVector<float64_t> w(n_feats);
		for (int32_t j=0; j<n_feats; j++)
			w[j] = all_w(j,i);
		float64_t c = all_c[i];
		auto machine = std::make_shared<LinearMachine>();
		machine->set_w(w);
		machine->set_bias(c);
		m_machines.push_back(machine);
	}
	return true;
}
