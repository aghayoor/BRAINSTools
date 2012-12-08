import argparse
from nipype.interfaces.freesurfer.base import Info

class ReconContainer(object):
    def __init__(self, *args, **kwds):
        import os.path.join, os.setenv
        from subprocess import check_call
        self.shell = '/bin/tsch'
        self.required = kwds['required']
        self._checkFiles()
        for key, val in kwargs.iteritems():
            assert( key in self.__class__.__allowed )
            setattr(self, key, val)
        self.FS_SCRIPT = os.path.join(self.FREESURFER_HOME, self.FS_SCRIPT)
        self._validateEnv()

    def setupEnv(self):
        os.setenv("FREESURFER_HOME", self.FREESURFER_HOME)
        sp.check_call(["source" self.FS_SCRIPT], shell=self.shell)
        os.setenv("SUBJECTS_DIR", self.FS_SUBJECT_DIR)

    def _checkFiles(self):
        import os
        for fname in self.required:
            fullname = os.path.join(os.getcwd(), fname)
            if not os.path.exists(fullname):
                self._touchFile(fullname)
            assert os.path.exists(fullname), "Required file does not exist and could not be created: %s" % fullname

    def _touchFile(self, fullname):
        import subprocess as sp
        sp.check_call(['touch' fullname])

    def _validateEnv(self):
        import os
        assert os.path.exist(self.FREESURFER_HOME), "FREESURFER_HOME doesn't exist: %s" % self.FREESURFER_HOME
        assert os.path.exist(self.FS_SUBJECTS_DIR), "SUBJECTS_DIR doesn't exist: %s" % self.FS_SUBJECTS_DIR
        assert os.path.exist(self.FS_SCRIPT, "FREESURFER script doesn't exist: %s" % self.FS_SCRIPT)

def autoRecon1(*arg, **kwds):
    """
    TODO: Traditional Freesurfer stage (for comparison)
    """
    pass

def normalizeWM(t1, wm_prob):
    """This function will compute the mean value of wm and rescale the image so that the mean WM=100"""
    WM_THRESHOLD = 0.66
    wm_mask = sitk.BinaryThreshold(wm_prob, WM_THRESHOLD)
    ls = sitk.LabelStatisticsImageFilter()
    ls.Execute(t1, wm_mask)
    wm_value = 1
    myMeasurementMap = ls.GetMeasurementMap(wm_value)
    MeanWM = myMeasurementMap['Mean']
    t1_new = sitk.Cast(sitk.Cast(t1, sitk.sitkFloat32) * 100.0 / MeanWM, sitk.sitkUInt8)
    return t1_new

def baw_Recon1(*arg, **kwds):
    t1 = sitk.ReadImage(kwds['t1Image'])
    t2 = sitk.ReadImage(kwds['t2Image'])
    wm = sitk.ReadImage(kwds['wmProbImage'])
    brain = sitk.ReadImage(kwds['brainLabel'])
    t1_new = normalizeWM(t1, wm)
    clipping = sitk.BinaryThreshold(brain, 1, 1000000) - sitk.BinaryThreshold(brain, 5, 5)
    clipped = t1_new * clipping

def autoRecon2(*arg, **kwds):
    import os.path.sep as sep
    import subprocess as sp
    dir1 = 'orig' + sep
    dir2 = 'transforms' + sep
    required = (dir1 + '001.mgz',
                dir1 + '002.mgz',
                'rawavg.mgz',
                'orig.mgz',
                dir2 + 'talairach.auto.xfm'
                dir2 + 'talairach.xfm',
                dir2 + 'talairach_avi.log',
                'nu.mgz',
                'T1.mgz',
                dir2 + 'talairach_with_skull.lta',
                'brainmask.auto.mgz',
                'brainmask.mgz')
    container = ReconContainer(required=required. **kwds)
    subj_id = kwds['subjID'] # TODO: Potential security risk - need to sanitize
    with open('autoRecon2.log', 'w') as fID:
        container.setupEnv()
        sp.check_call(['recon-all', '-autorecon2', '-debug', '-subjid', subj_id], stdout=fID, stderr=sp.STDOUT, shell=container.shell)

def autoRecon3(*arg, **kwds):
    import os.path.sep
    dir1 = '..' + sep + 'scripts' + sep
    dir2 = 'transforms' + sep
    required = (dir2 + 'talairach.lta',
                'norm.mgz',
                dir2 + 'talairach.m3z',
                dir2 + 'talairach.m3z.inv.x.mgz',
                dir2 + 'talairach.m3z.inv.y.mgz',
                dir2 + 'talairach.m3z.inv.z.mgz',
                'nu_noneck.mgz',
                dir2 + 'talairach_with_skull_2.lta',
                'aseg.auto_noCCseg.mgz',
                'aseg.auto.mgz',
                'aseg.mgz',
                'brain.mgz',
                'brain.finalsurfs.mgz',
                'wm.seg.mgz',
                'wm.asegedit.mgz',
                'wm.mgz',
                'filled.mgz',
                dir1 + 'ponscc.cut.log',
                'filled-pretess255.mgz', # Removed by Freesurfer
                'lh.orig.nofix',
                'filled-pretess127.mgz', # Removed by Freesurfer
                'rh.orig.nofix',
                'lh.smoothwm.nofix', 'rh.smoothwm.nofix',
                'lh.inflated.nofix', 'rh.inflated.nofix',
                'lh.qsphere.nofix',  'rh.qsphere.nofix',
                'lh.orig',           'rh.orig',
                'lh.inflated',       'rh.inflated', # Removed by Freesurfer
                'lh.white',          'rh.white',
                'lh.curv',           'rh.curv',
                'lh.area',           'rh.area',
                'lh.cortex.label',   'rh.cortex.label',
                'lh.smoothwm',       'rh.smoothwm',
                'lh.sulc',           'rh.sulc',
                'lh.inflated.H',     'rh.inflated.H',
                'lh.inflated.K'      'rh.inflated.K'
        )
    container = ReconContainer(required=required, **kwds)
    subj_id = kwds['subjID'] # TODO: Potential security risk - need to sanitize
    with open('autoRecon3.log', 'w') as fID:
        container.setupEnv()
        sp.check_call(['recon-all', '-autorecon3', '-debug', '-subjid', subj_id], stdout=fID, stderr=sp.STDOUT, shell=container.shell)

def run(*arg, **kwds):
    import os
    os.environ['SUBJECTS_DIR'] = kwds['FS_SUBJECTS_DIR']
    os.environ['FREESURFER_HOME'] = kwds['FREESURFER_HOME']

    # autoRecon1(*arg, **kwds)
    baw_Recon1(*arg, **kwds)
    autoRecon2(*arg, **kwds)
    autoRecon3(*arg, **kwds)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="""
                                     """

    """DELETE LATER: This is an just example of the commands required to run FreeSurfer recon-all:
    /bin/tcsh
    setenv FREESURFER_HOME /ipldev/sharedopt/20110601/MacOSX_10.6/freesurfer
    source ${FREESURFER_HOME}/FreeSurferEnv.csh
    setenv SUBJECTS_DIR /IPLlinux/raid0/homes/jforbes/freesurfer/recon-all/autorecon1_copy
    recon-all -make all -subjid 0074_24832

    Link to recon-all i/o table:
    http://surfer.nmr.mgh.harvard.edu/fswiki/ReconAllDevTable

    """
                                     )
    parser.add_argument('-t1', '--T1image', action='store', dest='t1Image', help='Original T1 image')
    parser.add_argument('-t2', '--T2image', action='store', dest='t2Image', help='Original T2 image')
    parser.add_argument('-h', '--FSHomeDir', action='store', dest='FREESURFER_HOME',
                        default='/ipldev/sharedopt/20110601/MacOSX_10.6/freesurfer',
                        help='Location of FreeSurfer (differs for Mac and Linux environments')
    parser.add_argument('-d', '--FSSubjDir', action='store', dest='FS_SUBJECTS_DIR', help='FreeSurfer subjects directory')
    parser.add_argument('-s', '--FSSource', action='store', dest='FS_SCRIPT',
                        default='FreeSurferEnv.csh', help='')
    parser.add_argument('-i', '--SubjID', action='store', dest='subjID', help='Subject_Session')
    parser.add_argument('-b', '--BrainLabel', action='store', dest='brainLabel',
                        help='The normalized T1 image with the skull removed. Normalized 0-110 where white matter=110.')
    parser.add_argument('-w', '--WMProb', action='store', dest='wmProbImage', help='')
    kwds = vars(parser.parse_args())
    return runAutoRecon(**kwds)