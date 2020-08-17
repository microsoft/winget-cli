// Copyright (c) Microsoft Corporation.
// Licensed under the MIT License.

namespace AppInstallerCLIE2ETests
{
    using NUnit.Framework;
    using System.IO;

    public class TextSupport
    {
        private const string TextSupportTestSourceUrl = @"https://localhost:5001/TestKit/";
        private const string TextSupportSourceName = @"TextSupportTestSource";

        [SetUp]
        public void Setup()
        {
            Assert.AreEqual(Constants.ErrorCode.S_OK, TestCommon.RunAICLICommand("source add", $"{TextSupportSourceName} {TextSupportTestSourceUrl}").ExitCode);
        }

        [TearDown]
        public void TearDown()
        {
            TestCommon.RunAICLICommand("source remove", TextSupportSourceName);
            TestCommon.WaitForDeploymentFinish();
        }

        [Test]
        public void VerifyGB18030Support()
        {
            var result = TestCommon.RunAICLICommand("show", $"TestGB18030SupportInstaller -s {TextSupportSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("丂令龥€￥ 㐀㲷䶵 𠀀𠀁𠀂"));
            Assert.True(result.StdOut.Contains("İkşzlerAçık芲偁ＡＢＣ巢für नमस्ते กุ้งจิ้яЧчŠš𠀀𠀁𠀂"));
        }

        [Test]
        public void VerifyDirectionalitySupport()
        {
            var result = TestCommon.RunAICLICommand("show", $"TestDirectionalitySupportInstaller -s {TextSupportSourceName}");
            Assert.AreEqual(Constants.ErrorCode.S_OK, result.ExitCode);
            Assert.True(result.StdOut.Contains("أنا اختبار إدخال النص في لغات مختلفة 01 لأحد منتجات Microsoft"));
        }
    }
}
